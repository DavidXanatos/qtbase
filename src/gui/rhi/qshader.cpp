// Copyright (C) 2019 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qshader_p_p.h"
#include <QDataStream>
#include <QBuffer>

QT_BEGIN_NAMESPACE

/*!
    \class QShader
    \internal
    \inmodule QtGui

    \brief Contains multiple versions of a shader translated to multiple shading languages,
    together with reflection metadata.

    QShader is the entry point to shader code in the graphics API agnostic
    Qt world. Instead of using GLSL shader sources, as was the custom with Qt
    5.x, new graphics systems with backends for multiple graphics APIs, such
    as, Vulkan, Metal, Direct3D, and OpenGL, take QShader as their input
    whenever a shader needs to be specified.

    A QShader instance is empty and thus invalid by default. To get a useful
    instance, the two typical methods are:

    \list

    \li Generate the contents offline, during build time or earlier, using the
    \c qsb command line tool. The result is a binary file that is shipped with
    the application, read via QIODevice::readAll(), and then deserialized via
    fromSerialized(). For more information, see QShaderBaker.

    \li Generate at run time via QShaderBaker. This is an expensive operation,
    but allows applications to use user-provided or dynamically generated
    shader source strings.

    \endlist

    When used together with the Qt Rendering Hardware Interface and its
    classes, like QRhiGraphicsPipeline, no further action is needed from the
    application's side as these classes are prepared to consume a QShader
    whenever a shader needs to be specified for a given stage of the graphics
    pipeline.

    Alternatively, applications can access

    \list

    \li the source or byte code for any of the shading language versions that
    are included in the QShader,

    \li the name of the entry point for the shader,

    \li the reflection metadata containing a description of the shader's
    inputs, outputs and resources like uniform blocks. This is essential when
    an application or framework needs to discover the inputs of a shader at
    runtime due to not having advance knowledge of the vertex attributes or the
    layout of the uniform buffers used by the shader.

    \endlist

    QShader makes no assumption about the shading language that was used
    as the source for generating the various versions and variants that are
    included in it.

    QShader uses implicit sharing similarly to many core Qt types, and so
    can be returned or passed by value. Detach happens implicitly when calling
    a setter.

    For reference, QRhi expects that a QShader suitable for all its
    backends contains at least the following:

    \list

    \li SPIR-V 1.0 bytecode suitable for Vulkan 1.0 or newer

    \li GLSL/ES 100 source code suitable for OpenGL ES 2.0 or newer

    \li GLSL 120 source code suitable for OpenGL 2.1

    \li HLSL Shader Model 5.0 source code or the corresponding DXBC bytecode suitable for Direct3D 11

    \li Metal Shading Language 1.2 source code or the corresponding bytecode suitable for Metal

    \endlist

    \sa QShaderBaker
 */

/*!
    \enum QShader::Stage
    Describes the stage of the graphics pipeline the shader is suitable for.

    \value VertexStage Vertex shader
    \value TessellationControlStage Tessellation control (hull) shader
    \value TessellationEvaluationStage Tessellation evaluation (domain) shader
    \value GeometryStage Geometry shader
    \value FragmentStage Fragment (pixel) shader
    \value ComputeStage Compute shader
 */

/*!
    \class QShaderVersion
    \internal
    \inmodule QtGui

    \brief Specifies the shading language version.

    While languages like SPIR-V or the Metal Shading Language use traditional
    version numbers, shaders for other APIs can use slightly different
    versioning schemes. All those are mapped to a single version number in
    here, however. For HLSL, the version refers to the Shader Model version,
    like 5.0, 5.1, or 6.0. For GLSL an additional flag is needed to choose
    between GLSL and GLSL/ES.

    Below is a list with the most common examples of shader versions for
    different graphics APIs:

    \list

    \li Vulkan (SPIR-V): 100
    \li OpenGL: 120, 330, 440, etc.
    \li OpenGL ES: 100 with GlslEs, 300 with GlslEs, etc.
    \li Direct3D: 50, 51, 60
    \li Metal: 12, 20
    \endlist

    A default constructed QShaderVersion contains a version of 100 and no
    flags set.
 */

/*!
    \enum QShaderVersion::Flag

    Describes the flags that can be set.

    \value GlslEs Indicates that GLSL/ES is meant in combination with GlslShader
 */

/*!
    \class QShaderKey
    \internal
    \inmodule QtGui

    \brief Specifies the shading language, the version with flags, and the variant.

    A default constructed QShaderKey has source set to SpirvShader and
    sourceVersion set to 100. sourceVariant defaults to StandardShader.
 */

/*!
    \enum QShader::Source
    Describes what kind of shader code an entry contains.

    \value SpirvShader SPIR-V
    \value GlslShader GLSL
    \value HlslShader HLSL
    \value DxbcShader Direct3D bytecode (HLSL compiled by \c fxc)
    \value MslShader Metal Shading Language
    \value DxilShader Direct3D bytecode (HLSL compiled by \c dxc)
    \value MetalLibShader Pre-compiled Metal bytecode
 */

/*!
    \enum QShader::Variant
    Describes what kind of shader code an entry contains.

    \value StandardShader The normal, unmodified version of the shader code.
    \value BatchableVertexShader Vertex shader rewritten to be suitable for Qt Quick scenegraph batching.
 */

/*!
    \class QShaderCode
    \internal
    \inmodule QtGui

    \brief Contains source or binary code for a shader and additional metadata.

    When shader() is empty after retrieving a QShaderCode instance from
    QShader, it indicates no shader code was found for the requested key.
 */

/*!
    Constructs a new, empty (and thus invalid) QShader instance.
 */
QShader::QShader()
    : d(new QShaderPrivate)
{
}

/*!
    \internal
 */
void QShader::detach()
{
    qAtomicDetach(d);
}

/*!
    \internal
 */
QShader::QShader(const QShader &other)
    : d(other.d)
{
    d->ref.ref();
}

/*!
    \internal
 */
QShader &QShader::operator=(const QShader &other)
{
    qAtomicAssign(d, other.d);
    return *this;
}

/*!
    Destructor.
 */
QShader::~QShader()
{
    if (!d->ref.deref())
        delete d;
}

/*!
    \return true if the QShader contains at least one shader version.
 */
bool QShader::isValid() const
{
    return !d->shaders.isEmpty();
}

/*!
    \return the pipeline stage the shader is meant for.
 */
QShader::Stage QShader::stage() const
{
    return d->stage;
}

/*!
    Sets the pipeline \a stage.
 */
void QShader::setStage(Stage stage)
{
    if (stage != d->stage) {
        detach();
        d->stage = stage;
    }
}

/*!
    \return the reflection metadata for the shader.
 */
QShaderDescription QShader::description() const
{
    return d->desc;
}

/*!
    Sets the reflection metadata to \a desc.
 */
void QShader::setDescription(const QShaderDescription &desc)
{
    detach();
    d->desc = desc;
}

/*!
    \return the list of available shader versions
 */
QList<QShaderKey> QShader::availableShaders() const
{
    return d->shaders.keys().toVector();
}

/*!
    \return the source or binary code for a given shader version specified by \a key.
 */
QShaderCode QShader::shader(const QShaderKey &key) const
{
    return d->shaders.value(key);
}

/*!
    Stores the source or binary \a shader code for a given shader version specified by \a key.
 */
void QShader::setShader(const QShaderKey &key, const QShaderCode &shader)
{
    if (d->shaders.value(key) == shader)
        return;

    detach();
    d->shaders[key] = shader;
}

/*!
    Removes the source or binary shader code for a given \a key.
    Does nothing when not found.
 */
void QShader::removeShader(const QShaderKey &key)
{
    auto it = d->shaders.find(key);
    if (it == d->shaders.end())
        return;

    detach();
    d->shaders.erase(it);
}

static void writeShaderKey(QDataStream *ds, const QShaderKey &k)
{
    *ds << int(k.source());
    *ds << k.sourceVersion().version();
    *ds << k.sourceVersion().flags();
    *ds << int(k.sourceVariant());
}

/*!
    \return a serialized binary version of all the data held by the
    QShader, suitable for writing to files or other I/O devices.

    \sa fromSerialized()
 */
QByteArray QShader::serialized() const
{
    QBuffer buf;
    QDataStream ds(&buf);
    ds.setVersion(QDataStream::Qt_5_10);
    if (!buf.open(QIODevice::WriteOnly))
        return QByteArray();

    ds << QShaderPrivate::QSB_VERSION;
    ds << int(d->stage);
    d->desc.serialize(&ds);
    ds << int(d->shaders.count());
    for (auto it = d->shaders.cbegin(), itEnd = d->shaders.cend(); it != itEnd; ++it) {
        const QShaderKey &k(it.key());
        writeShaderKey(&ds, k);
        const QShaderCode &shader(d->shaders.value(k));
        ds << shader.shader();
        ds << shader.entryPoint();
    }
    ds << int(d->bindings.count());
    for (auto it = d->bindings.cbegin(), itEnd = d->bindings.cend(); it != itEnd; ++it) {
        const QShaderKey &k(it.key());
        writeShaderKey(&ds, k);
        const NativeResourceBindingMap &map(it.value());
        ds << int(map.count());
        for (auto mapIt = map.cbegin(), mapItEnd = map.cend(); mapIt != mapItEnd; ++mapIt) {
            ds << mapIt.key();
            ds << mapIt.value().first;
            ds << mapIt.value().second;
        }
    }
    ds << int(d->combinedImageMap.count());
    for (auto it = d->combinedImageMap.cbegin(), itEnd = d->combinedImageMap.cend(); it != itEnd; ++it) {
        const QShaderKey &k(it.key());
        writeShaderKey(&ds, k);
        const SeparateToCombinedImageSamplerMappingList &list(it.value());
        ds << int(list.count());
        for (auto listIt = list.cbegin(), listItEnd = list.cend(); listIt != listItEnd; ++listIt) {
            ds << listIt->combinedSamplerName;
            ds << listIt->textureBinding;
            ds << listIt->samplerBinding;
        }
    }

    return qCompress(buf.buffer());
}

static void readShaderKey(QDataStream *ds, QShaderKey *k)
{
    int intVal;
    *ds >> intVal;
    k->setSource(QShader::Source(intVal));
    QShaderVersion ver;
    *ds >> intVal;
    ver.setVersion(intVal);
    *ds >> intVal;
    ver.setFlags(QShaderVersion::Flags(intVal));
    k->setSourceVersion(ver);
    *ds >> intVal;
    k->setSourceVariant(QShader::Variant(intVal));
}

/*!
    Creates a new QShader instance from the given \a data.

    \sa serialized()
  */
QShader QShader::fromSerialized(const QByteArray &data)
{
    QByteArray udata = qUncompress(data);
    QBuffer buf(&udata);
    QDataStream ds(&buf);
    ds.setVersion(QDataStream::Qt_5_10);
    if (!buf.open(QIODevice::ReadOnly))
        return QShader();

    QShader bs;
    QShaderPrivate *d = QShaderPrivate::get(&bs);
    Q_ASSERT(d->ref.loadRelaxed() == 1); // must be detached
    int intVal;
    ds >> intVal;
    d->qsbVersion = intVal;
    if (d->qsbVersion != QShaderPrivate::QSB_VERSION
            && d->qsbVersion != QShaderPrivate::QSB_VERSION_WITHOUT_SEPARATE_IMAGES_AND_SAMPLERS
            && d->qsbVersion != QShaderPrivate::QSB_VERSION_WITHOUT_VAR_ARRAYDIMS
            && d->qsbVersion != QShaderPrivate::QSB_VERSION_WITH_CBOR
            && d->qsbVersion != QShaderPrivate::QSB_VERSION_WITH_BINARY_JSON
            && d->qsbVersion != QShaderPrivate::QSB_VERSION_WITHOUT_BINDINGS)
    {
        qWarning("Attempted to deserialize QShader with unknown version %d.", d->qsbVersion);
        return QShader();
    }

    ds >> intVal;
    d->stage = Stage(intVal);
    if (d->qsbVersion > QShaderPrivate::QSB_VERSION_WITH_CBOR) {
        d->desc = QShaderDescription::deserialize(&ds, d->qsbVersion);
    } else if (d->qsbVersion > QShaderPrivate::QSB_VERSION_WITH_BINARY_JSON) {
        qWarning("Can no longer load QShaderDescription from CBOR.");
        d->desc = QShaderDescription();
    } else {
        qWarning("Can no longer load QShaderDescription from binary JSON.");
        d->desc = QShaderDescription();
    }
    int count;
    ds >> count;
    for (int i = 0; i < count; ++i) {
        QShaderKey k;
        readShaderKey(&ds, &k);
        QShaderCode shader;
        QByteArray s;
        ds >> s;
        shader.setShader(s);
        ds >> s;
        shader.setEntryPoint(s);
        d->shaders[k] = shader;
    }

    if (d->qsbVersion > QShaderPrivate::QSB_VERSION_WITHOUT_BINDINGS) {
        ds >> count;
        for (int i = 0; i < count; ++i) {
            QShaderKey k;
            readShaderKey(&ds, &k);
            NativeResourceBindingMap map;
            int mapSize;
            ds >> mapSize;
            for (int b = 0; b < mapSize; ++b) {
                int binding;
                ds >> binding;
                int firstNativeBinding;
                ds >> firstNativeBinding;
                int secondNativeBinding;
                ds >> secondNativeBinding;
                map.insert(binding, { firstNativeBinding, secondNativeBinding });
            }
            d->bindings.insert(k, map);
        }
    }

    if (d->qsbVersion > QShaderPrivate::QSB_VERSION_WITHOUT_SEPARATE_IMAGES_AND_SAMPLERS) {
        ds >> count;
        for (int i = 0; i < count; ++i) {
            QShaderKey k;
            readShaderKey(&ds, &k);
            SeparateToCombinedImageSamplerMappingList list;
            int listSize;
            ds >> listSize;
            for (int b = 0; b < listSize; ++b) {
                QByteArray combinedSamplerName;
                ds >> combinedSamplerName;
                int textureBinding;
                ds >> textureBinding;
                int samplerBinding;
                ds >> samplerBinding;
                list.append({ combinedSamplerName, textureBinding, samplerBinding });
            }
            d->combinedImageMap.insert(k, list);
        }
    }

    return bs;
}

QShaderVersion::QShaderVersion(int v, Flags f)
    : m_version(v), m_flags(f)
{
}

QShaderCode::QShaderCode(const QByteArray &code, const QByteArray &entry)
    : m_shader(code), m_entryPoint(entry)
{
}

QShaderKey::QShaderKey(QShader::Source s,
                       const QShaderVersion &sver,
                       QShader::Variant svar)
    : m_source(s),
      m_sourceVersion(sver),
      m_sourceVariant(svar)
{
}

/*!
    Returns \c true if the two QShader objects \a lhs and \a rhs are equal,
    meaning they are for the same stage with matching sets of shader source or
    binary code.

    \relates QShader
 */
bool operator==(const QShader &lhs, const QShader &rhs) noexcept
{
    return lhs.d->stage == rhs.d->stage
            && lhs.d->shaders == rhs.d->shaders;
    // do not bother with desc and bindings, if the shader code is the same, the description must match too
}

/*!
    \internal
    \fn bool operator!=(const QShader &lhs, const QShader &rhs)

    Returns \c false if the values in the two QShader objects \a a and \a b
    are equal; otherwise returns \c true.

    \relates QShader
 */

/*!
    Returns the hash value for \a s, using \a seed to seed the calculation.

    \relates QShader
 */
size_t qHash(const QShader &s, size_t seed) noexcept
{
    QtPrivate::QHashCombine hash;
    seed = hash(seed, s.stage());
    seed = qHashRange(s.d->shaders.keyValueBegin(),
                      s.d->shaders.keyValueEnd(),
                      seed);
    return seed;
}

/*!
    Returns \c true if the two QShaderVersion objects \a lhs and \a rhs are
    equal.

    \relates QShaderVersion
 */
bool operator==(const QShaderVersion &lhs, const QShaderVersion &rhs) noexcept
{
    return lhs.version() == rhs.version() && lhs.flags() == rhs.flags();
}

#ifdef Q_OS_INTEGRITY
size_t qHash(const QShaderVersion &s, size_t seed) noexcept
{
    return qHashMulti(seed, s.version(), s.flags());
}
#endif

/*!
    Establishes a sorting order between the two QShaderVersion \a lhs and \a rhs.

    \relates QShaderVersion
 */
bool operator<(const QShaderVersion &lhs, const QShaderVersion &rhs) noexcept
{
    if (lhs.version() < rhs.version())
        return true;

    if (lhs.version() == rhs.version())
        return int(lhs.flags()) < int(rhs.flags());

    return false;
}

/*!
    \internal
    \fn bool operator!=(const QShaderVersion &lhs, const QShaderVersion &rhs)

    Returns \c false if the values in the two QShaderVersion objects \a a
    and \a b are equal; otherwise returns \c true.

    \relates QShaderVersion
 */

/*!
    Returns \c true if the two QShaderKey objects \a lhs and \a rhs are equal.

    \relates QShaderKey
 */
bool operator==(const QShaderKey &lhs, const QShaderKey &rhs) noexcept
{
    return lhs.source() == rhs.source() && lhs.sourceVersion() == rhs.sourceVersion()
            && lhs.sourceVariant() == rhs.sourceVariant();
}

/*!
    Establishes a sorting order between the two keys \a lhs and \a rhs.

    \relates QShaderKey
 */
bool operator<(const QShaderKey &lhs, const QShaderKey &rhs) noexcept
{
    if (int(lhs.source()) < int(rhs.source()))
        return true;

    if (int(lhs.source()) == int(rhs.source())) {
        if (lhs.sourceVersion() < rhs.sourceVersion())
            return true;
        if (lhs.sourceVersion() == rhs.sourceVersion()) {
            if (int(lhs.sourceVariant()) < int(rhs.sourceVariant()))
                return true;
        }
    }

    return false;
}

/*!
    \internal
    \fn bool operator!=(const QShaderKey &lhs, const QShaderKey &rhs)

    Returns \c false if the values in the two QShaderKey objects \a a
    and \a b are equal; otherwise returns \c true.

    \relates QShaderKey
 */

/*!
    Returns the hash value for \a k, using \a seed to seed the calculation.

    \relates QShaderKey
 */
size_t qHash(const QShaderKey &k, size_t seed) noexcept
{
    return qHashMulti(seed,
                      k.source(),
                      k.sourceVersion().version(),
                      k.sourceVersion().flags(),
                      k.sourceVariant());
}

/*!
    Returns \c true if the two QShaderCode objects \a lhs and \a rhs are equal.

    \relates QShaderCode
 */
bool operator==(const QShaderCode &lhs, const QShaderCode &rhs) noexcept
{
    return lhs.shader() == rhs.shader() && lhs.entryPoint() == rhs.entryPoint();
}

/*!
    \internal
    \fn bool operator!=(const QShaderCode &lhs, const QShaderCode &rhs)

    Returns \c false if the values in the two QShaderCode objects \a a
    and \a b are equal; otherwise returns \c true.

    \relates QShaderCode
 */

/*!
    Returns the hash value for \a k, using \a seed to seed the calculation.

    \relates QShaderCode
 */
size_t qHash(const QShaderCode &k, size_t seed) noexcept
{
    return qHash(k.shader(), seed);
}

#ifndef QT_NO_DEBUG_STREAM
QDebug operator<<(QDebug dbg, const QShader &bs)
{
    const QShaderPrivate *d = bs.d;
    QDebugStateSaver saver(dbg);

    dbg.nospace() << "QShader("
                  << "stage=" << d->stage
                  << " shaders=" << d->shaders.keys()
                  << " desc.isValid=" << d->desc.isValid()
                  << ')';

    return dbg;
}

QDebug operator<<(QDebug dbg, const QShaderKey &k)
{
    QDebugStateSaver saver(dbg);
    dbg.nospace() << "ShaderKey(" << k.source()
                  << " " << k.sourceVersion()
                  << " " << k.sourceVariant() << ")";
    return dbg;
}

QDebug operator<<(QDebug dbg, const QShaderVersion &v)
{
    QDebugStateSaver saver(dbg);
    dbg.nospace() << "Version(" << v.version() << " " << v.flags() << ")";
    return dbg;
}
#endif // QT_NO_DEBUG_STREAM

/*!
    \typedef QShader::NativeResourceBindingMap

    Synonym for QHash<int, QPair<int, int>>.

    The resource binding model QRhi assumes is based on SPIR-V. This means that
    uniform buffers, storage buffers, combined image samplers, and storage
    images share a common binding point space. The binding numbers in
    QShaderDescription and QRhiShaderResourceBinding are expected to match the
    \c binding layout qualifier in the Vulkan-compatible GLSL shader.

    Graphics APIs other than Vulkan may use a resource binding model that is
    not fully compatible with this. The generator of the shader code translated
    from SPIR-V may choose not to take the SPIR-V binding qualifiers into
    account, for various reasons. This is the case with the Metal backend of
    SPIRV-Cross, for example. In addition, even when an automatic, implicit
    translation is mostly possible (e.g. by using SPIR-V binding points as HLSL
    resource register indices), assigning resource bindings without being
    constrained by the SPIR-V binding points can lead to better results.

    Therefore, a QShader may expose an additional map that describes what the
    native binding point for a given SPIR-V binding is. The QRhi backends, for
    which this is relevant, are expected to use this map automatically, as
    appropriate. The value is a pair, because combined image samplers may map
    to two native resources (a texture and a sampler) in some shading
    languages. In that case the second value refers to the sampler.

    \note The native binding may be -1, in case there is no active binding for
    the resource in the shader. (for example, there is a uniform block
    declared, but it is not used in the shader code) The map is always
    complete, meaning there is an entry for all declared uniform blocks,
    storage blocks, image objects, and combined samplers, but the value will be
    -1 for those that are not actually referenced in the shader functions.
*/

/*!
    \return the native binding map for \a key. The map is empty if no mapping
    is available for \a key (for example, because the map is not applicable for
    the API and shading language described by \a key).
 */
QShader::NativeResourceBindingMap QShader::nativeResourceBindingMap(const QShaderKey &key) const
{
    auto it = d->bindings.constFind(key);
    if (it == d->bindings.cend())
        return {};

    return it.value();
}

/*!
    Stores the given native resource binding \a map associated with \a key.

    \sa nativeResourceBindingMap()
 */
void QShader::setResourceBindingMap(const QShaderKey &key, const NativeResourceBindingMap &map)
{
    detach();
    d->bindings[key] = map;
}

/*!
    Removes the native resource binding map for \a key.
 */
void QShader::removeResourceBindingMap(const QShaderKey &key)
{
    auto it = d->bindings.find(key);
    if (it == d->bindings.end())
        return;

    detach();
    d->bindings.erase(it);
}

/*!
    \typedef QShader::SeparateToCombinedImageSamplerMappingList

    Synonym for QList<QShader::SeparateToCombinedImageSamplerMapping>.
 */

/*!
    \struct QShader::SeparateToCombinedImageSamplerMapping

    Describes a mapping from a traditional combined image sampler uniform to
    binding points for a separate texture and sampler.

    For example, if \c combinedImageSampler is \c{"_54"}, \c textureBinding is
    \c 1, and \c samplerBinding is \c 2, this means that the GLSL shader code
    contains a \c sampler2D (or sampler3D, etc.) uniform with the name of
    \c{_54} which corresponds to two separate resource bindings (\c 1 and \c 2)
    in the original shader.
 */

/*!
    \return the combined image sampler mapping list for \a key, or an empty
    list if there is no data available for \a key, for example because such a
    mapping is not applicable for the shading language.
 */
QShader::SeparateToCombinedImageSamplerMappingList QShader::separateToCombinedImageSamplerMappingList(const QShaderKey &key) const
{
    auto it = d->combinedImageMap.constFind(key);
    if (it == d->combinedImageMap.cend())
        return {};

    return it.value();
}

/*!
    Stores the given combined image sampler mapping \a list associated with \a key.

    \sa separateToCombinedImageSamplerMappingList()
 */
void QShader::setSeparateToCombinedImageSamplerMappingList(const QShaderKey &key,
                                                           const SeparateToCombinedImageSamplerMappingList &list)
{
    detach();
    d->combinedImageMap[key] = list;
}

/*!
    Removes the combined image sampler mapping list for \a key.
 */
void QShader::removeSeparateToCombinedImageSamplerMappingList(const QShaderKey &key)
{
    auto it = d->combinedImageMap.find(key);
    if (it == d->combinedImageMap.end())
        return;

    detach();
    d->combinedImageMap.erase(it);
}

QT_END_NAMESPACE
