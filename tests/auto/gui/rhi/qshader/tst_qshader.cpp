// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include <QTest>
#include <QFile>
#include <QBuffer>

#include <QtGui/private/qshaderdescription_p_p.h>
#include <QtGui/private/qshader_p_p.h>

class tst_QShader : public QObject
{
    Q_OBJECT

private slots:
    void serializeDeserialize();
    void simpleCompileCheckResults();
    void genVariants();
    void shaderDescImplicitSharing();
    void bakedShaderImplicitSharing();
    void sortedKeys();
    void mslResourceMapping();
    void serializeShaderDesc();
    void comparison();
    void loadV4();
    void manualShaderPackCreation();
    void loadV6WithSeparateImagesAndSamplers();
};

static QShader getShader(const QString &name)
{
    QFile f(name);
    if (f.open(QIODevice::ReadOnly))
        return QShader::fromSerialized(f.readAll());

    return QShader();
}

void tst_QShader::serializeDeserialize()
{
    QShader s = getShader(QLatin1String(":/data/texture_all_v4.frag.qsb"));
    QVERIFY(s.isValid());

    QByteArray data = s.serialized();
    QVERIFY(!data.isEmpty());

    QShader s2;
    QVERIFY(!s2.isValid());
    QVERIFY(s != s2);
    s2 = QShader::fromSerialized(data);
    QVERIFY(s2.isValid());
    QCOMPARE(s, s2);
}

void tst_QShader::simpleCompileCheckResults()
{
    QShader s = getShader(QLatin1String(":/data/color_spirv_v5.vert.qsb"));
    QVERIFY(s.isValid());
    QCOMPARE(QShaderPrivate::get(&s)->qsbVersion, 5);
    QCOMPARE(s.availableShaders().count(), 1);

    const QShaderCode shader = s.shader(QShaderKey(QShader::SpirvShader,
                                                   QShaderVersion(100)));
    QVERIFY(!shader.shader().isEmpty());
    QCOMPARE(shader.entryPoint(), QByteArrayLiteral("main"));

    const QShaderDescription desc = s.description();
    QVERIFY(desc.isValid());
    QCOMPARE(desc.inputVariables().count(), 2);
    for (const QShaderDescription::InOutVariable &v : desc.inputVariables()) {
        switch (v.location) {
        case 0:
            QCOMPARE(v.name, QByteArrayLiteral("position"));
            QCOMPARE(v.type, QShaderDescription::Vec4);
            break;
        case 1:
            QCOMPARE(v.name, QByteArrayLiteral("color"));
            QCOMPARE(v.type, QShaderDescription::Vec3);
            break;
        default:
            QFAIL(qPrintable(QStringLiteral("Bad location: %1").arg(v.location)));
            break;
        }
    }
    QCOMPARE(desc.outputVariables().count(), 1);
    for (const QShaderDescription::InOutVariable &v : desc.outputVariables()) {
        switch (v.location) {
        case 0:
            QCOMPARE(v.name, QByteArrayLiteral("v_color"));
            QCOMPARE(v.type, QShaderDescription::Vec3);
            break;
        default:
            QFAIL(qPrintable(QStringLiteral("Bad location: %1").arg(v.location)));
            break;
        }
    }
    QCOMPARE(desc.uniformBlocks().count(), 1);
    const QShaderDescription::UniformBlock blk = desc.uniformBlocks().first();
    QCOMPARE(blk.blockName, QByteArrayLiteral("buf"));
    QCOMPARE(blk.structName, QByteArrayLiteral("ubuf"));
    QCOMPARE(blk.size, 68);
    QCOMPARE(blk.binding, 0);
    QCOMPARE(blk.descriptorSet, 0);
    QCOMPARE(blk.members.count(), 2);
    for (int i = 0; i < blk.members.count(); ++i) {
        const QShaderDescription::BlockVariable v = blk.members[i];
        switch (i) {
        case 0:
            QCOMPARE(v.offset, 0);
            QCOMPARE(v.size, 64);
            QCOMPARE(v.name, QByteArrayLiteral("mvp"));
            QCOMPARE(v.type, QShaderDescription::Mat4);
            QCOMPARE(v.matrixStride, 16);
            break;
        case 1:
            QCOMPARE(v.offset, 64);
            QCOMPARE(v.size, 4);
            QCOMPARE(v.name, QByteArrayLiteral("opacity"));
            QCOMPARE(v.type, QShaderDescription::Float);
            break;
        default:
            QFAIL(qPrintable(QStringLiteral("Too many blocks: %1").arg(blk.members.count())));
            break;
        }
    }
}

void tst_QShader::genVariants()
{
    QShader s = getShader(QLatin1String(":/data/color_all_v5.vert.qsb"));
    // spirv, glsl 100, glsl 330, glsl 120, hlsl 50, msl 12
    // + batchable variants
    QVERIFY(s.isValid());
    QCOMPARE(QShaderPrivate::get(&s)->qsbVersion, 5);
    QCOMPARE(s.availableShaders().count(), 2 * 6);

    int batchableVariantCount = 0;
    int batchableGlslVariantCount = 0;
    for (const QShaderKey &key : s.availableShaders()) {
        if (key.sourceVariant() == QShader::BatchableVertexShader) {
            ++batchableVariantCount;
            if (key.source() == QShader::GlslShader) {
                ++batchableGlslVariantCount;
                const QByteArray src = s.shader(key).shader();
                QVERIFY(src.contains(QByteArrayLiteral("_qt_order * ")));
            }
        }
    }
    QCOMPARE(batchableVariantCount, 6);
    QCOMPARE(batchableGlslVariantCount, 3);
}

void tst_QShader::shaderDescImplicitSharing()
{
    QShader s = getShader(QLatin1String(":/data/color_spirv_v5.vert.qsb"));
    QVERIFY(s.isValid());
    QCOMPARE(QShaderPrivate::get(&s)->qsbVersion, 5);
    QCOMPARE(s.availableShaders().count(), 1);
    QVERIFY(s.availableShaders().contains(QShaderKey(QShader::SpirvShader, QShaderVersion(100))));

    QShaderDescription d0 = s.description();
    QVERIFY(d0.isValid());
    QCOMPARE(d0.inputVariables().count(), 2);
    QCOMPARE(d0.outputVariables().count(), 1);
    QCOMPARE(d0.uniformBlocks().count(), 1);

    QShaderDescription d1 = d0;
    QVERIFY(QShaderDescriptionPrivate::get(&d0) == QShaderDescriptionPrivate::get(&d1));
    QCOMPARE(d0.inputVariables().count(), 2);
    QCOMPARE(d0.outputVariables().count(), 1);
    QCOMPARE(d0.uniformBlocks().count(), 1);
    QCOMPARE(d1.inputVariables().count(), 2);
    QCOMPARE(d1.outputVariables().count(), 1);
    QCOMPARE(d1.uniformBlocks().count(), 1);
    QCOMPARE(d0, d1);

    d1.detach();
    QVERIFY(QShaderDescriptionPrivate::get(&d0) != QShaderDescriptionPrivate::get(&d1));
    QCOMPARE(d0.inputVariables().count(), 2);
    QCOMPARE(d0.outputVariables().count(), 1);
    QCOMPARE(d0.uniformBlocks().count(), 1);
    QCOMPARE(d1.inputVariables().count(), 2);
    QCOMPARE(d1.outputVariables().count(), 1);
    QCOMPARE(d1.uniformBlocks().count(), 1);
    QCOMPARE(d0, d1);

    d1 = QShaderDescription();
    QVERIFY(d0 != d1);
}

void tst_QShader::bakedShaderImplicitSharing()
{
    QShader s0 = getShader(QLatin1String(":/data/color_spirv_v5.vert.qsb"));
    QVERIFY(s0.isValid());
    QCOMPARE(QShaderPrivate::get(&s0)->qsbVersion, 5);
    QCOMPARE(s0.availableShaders().count(), 1);
    QVERIFY(s0.availableShaders().contains(QShaderKey(QShader::SpirvShader, QShaderVersion(100))));

    {
        QShader s1 = s0;
        QVERIFY(QShaderPrivate::get(&s0) == QShaderPrivate::get(&s1));
        QCOMPARE(s0.availableShaders().count(), 1);
        QVERIFY(s0.availableShaders().contains(QShaderKey(QShader::SpirvShader, QShaderVersion(100))));
        QCOMPARE(s1.availableShaders().count(), 1);
        QVERIFY(s1.availableShaders().contains(QShaderKey(QShader::SpirvShader, QShaderVersion(100))));
        QCOMPARE(s0.stage(), s1.stage());
        QCOMPARE(s0, s1);

        s1.detach();
        QVERIFY(QShaderPrivate::get(&s0) != QShaderPrivate::get(&s1));
        QCOMPARE(s0.availableShaders().count(), 1);
        QVERIFY(s0.availableShaders().contains(QShaderKey(QShader::SpirvShader, QShaderVersion(100))));
        QCOMPARE(s1.availableShaders().count(), 1);
        QVERIFY(s1.availableShaders().contains(QShaderKey(QShader::SpirvShader, QShaderVersion(100))));
        QCOMPARE(s0.stage(), s1.stage());
        QCOMPARE(s0, s1);
    }

    {
        QShader s1 = s0;
        QVERIFY(QShaderPrivate::get(&s0) == QShaderPrivate::get(&s1));
        QCOMPARE(s0.stage(), s1.stage());

        s1.setStage(QShader::FragmentStage); // call a setter to trigger a detach
        QVERIFY(QShaderPrivate::get(&s0) != QShaderPrivate::get(&s1));
        QCOMPARE(s0.availableShaders().count(), 1);
        QVERIFY(s0.availableShaders().contains(QShaderKey(QShader::SpirvShader, QShaderVersion(100))));
        QCOMPARE(s1.availableShaders().count(), 1);
        QVERIFY(s1.availableShaders().contains(QShaderKey(QShader::SpirvShader, QShaderVersion(100))));
        QShaderDescription d0 = s0.description();
        QCOMPARE(d0.inputVariables().count(), 2);
        QCOMPARE(d0.outputVariables().count(), 1);
        QCOMPARE(d0.uniformBlocks().count(), 1);
        QShaderDescription d1 = s1.description();
        QCOMPARE(d1.inputVariables().count(), 2);
        QCOMPARE(d1.outputVariables().count(), 1);
        QCOMPARE(d1.uniformBlocks().count(), 1);
        QVERIFY(s0 != s1);
    }
}

void tst_QShader::sortedKeys()
{
    QShader s = getShader(QLatin1String(":/data/texture_all_v4.frag.qsb"));
    QVERIFY(s.isValid());
    QList<QShaderKey> availableShaders = s.availableShaders();
    QCOMPARE(availableShaders.count(), 7);
    std::sort(availableShaders.begin(), availableShaders.end());
    QCOMPARE(availableShaders, s.availableShaders());
}

void tst_QShader::mslResourceMapping()
{
    QShader s = getShader(QLatin1String(":/data/texture_all_v4.frag.qsb"));
    QVERIFY(s.isValid());
    QCOMPARE(QShaderPrivate::get(&s)->qsbVersion, 4);

    const QList<QShaderKey> availableShaders = s.availableShaders();
    QCOMPARE(availableShaders.count(), 7);
    QVERIFY(availableShaders.contains(QShaderKey(QShader::SpirvShader, QShaderVersion(100))));
    QVERIFY(availableShaders.contains(QShaderKey(QShader::MslShader, QShaderVersion(12))));
    QVERIFY(availableShaders.contains(QShaderKey(QShader::HlslShader, QShaderVersion(50))));
    QVERIFY(availableShaders.contains(QShaderKey(QShader::GlslShader, QShaderVersion(100, QShaderVersion::GlslEs))));
    QVERIFY(availableShaders.contains(QShaderKey(QShader::GlslShader, QShaderVersion(120))));
    QVERIFY(availableShaders.contains(QShaderKey(QShader::GlslShader, QShaderVersion(150))));
    QVERIFY(availableShaders.contains(QShaderKey(QShader::GlslShader, QShaderVersion(330))));

    QShader::NativeResourceBindingMap resMap =
            s.nativeResourceBindingMap(QShaderKey(QShader::GlslShader, QShaderVersion(330)));
    QVERIFY(resMap.isEmpty());

    // The Metal shader must come with a mapping table for binding points 0
    // (uniform buffer) and 1 (combined image sampler mapped to a texture and
    // sampler in the shader).
    resMap = s.nativeResourceBindingMap(QShaderKey(QShader::MslShader, QShaderVersion(12)));
    QVERIFY(!resMap.isEmpty());

    QCOMPARE(resMap.count(), 2);
    QCOMPARE(resMap.value(0).first, 0); // mapped to native buffer index 0
    QCOMPARE(resMap.value(1), qMakePair(0, 0)); // mapped to native texture index 0 and sampler index 0
}

void tst_QShader::serializeShaderDesc()
{
    // default constructed QShaderDescription
    {
        QShaderDescription desc;
        QVERIFY(!desc.isValid());

        QByteArray data;
        {
            QBuffer buf(&data);
            QDataStream ds(&buf);
            QVERIFY(buf.open(QIODevice::WriteOnly));
            desc.serialize(&ds);
        }
        QVERIFY(!data.isEmpty());

        {
            QBuffer buf(&data);
            QDataStream ds(&buf);
            QVERIFY(buf.open(QIODevice::ReadOnly));
            QShaderDescription desc2 = QShaderDescription::deserialize(&ds, QShaderPrivate::QSB_VERSION);
            QVERIFY(!desc2.isValid());
        }
    }

    // a QShaderDescription with inputs, outputs, uniform block and combined image sampler
    {
        QShader s = getShader(QLatin1String(":/data/texture_all_v4.frag.qsb"));
        QVERIFY(s.isValid());
        const QShaderDescription desc = s.description();
        QVERIFY(desc.isValid());

        QByteArray data;
        {
            QBuffer buf(&data);
            QDataStream ds(&buf);
            QVERIFY(buf.open(QIODevice::WriteOnly));
            desc.serialize(&ds);
        }
        QVERIFY(!data.isEmpty());

        {
            QShaderDescription desc2;
            QVERIFY(!desc2.isValid());
            QVERIFY(!(desc == desc2));
            QVERIFY(desc != desc2);
        }

        {
            QBuffer buf(&data);
            QDataStream ds(&buf);
            QVERIFY(buf.open(QIODevice::ReadOnly));
            QShaderDescription desc2 = QShaderDescription::deserialize(&ds, QShaderPrivate::QSB_VERSION);
            QVERIFY(desc2.isValid());
            QCOMPARE(desc, desc2);
        }
    }
}

void tst_QShader::comparison()
{
    // exercise QShader and QShaderDescription comparisons
    {
        QShader s1 = getShader(QLatin1String(":/data/texture_all_v4.frag.qsb"));
        QVERIFY(s1.isValid());
        QShader s2 = getShader(QLatin1String(":/data/color_all_v5.vert.qsb"));
        QVERIFY(s2.isValid());

        QVERIFY(s1.description().isValid());
        QVERIFY(s2.description().isValid());

        QVERIFY(s1 != s2);
        QVERIFY(s1.description() != s2.description());
    }

    {
        QShader s1 = getShader(QLatin1String(":/data/texture_all_v4.frag.qsb"));
        QVERIFY(s1.isValid());
        QShader s2 = getShader(QLatin1String(":/data/texture_all_v4.frag.qsb"));
        QVERIFY(s2.isValid());

        QVERIFY(s1.description().isValid());
        QVERIFY(s2.description().isValid());

        QVERIFY(s1 == s2);
        QVERIFY(s1.description() == s2.description());
    }
}

void tst_QShader::loadV4()
{
    // qsb version 4: QShaderDescription is serialized via QDataStream. Ensure the deserialized data is as expected.
    QShader s = getShader(QLatin1String(":/data/texture_all_v4.frag.qsb"));
    QVERIFY(s.isValid());
    QCOMPARE(QShaderPrivate::get(&s)->qsbVersion, 4);

    const QList<QShaderKey> availableShaders = s.availableShaders();
    QCOMPARE(availableShaders.count(), 7);
    QVERIFY(availableShaders.contains(QShaderKey(QShader::SpirvShader, QShaderVersion(100))));
    QVERIFY(availableShaders.contains(QShaderKey(QShader::MslShader, QShaderVersion(12))));
    QVERIFY(availableShaders.contains(QShaderKey(QShader::HlslShader, QShaderVersion(50))));
    QVERIFY(availableShaders.contains(QShaderKey(QShader::GlslShader, QShaderVersion(100, QShaderVersion::GlslEs))));
    QVERIFY(availableShaders.contains(QShaderKey(QShader::GlslShader, QShaderVersion(120))));
    QVERIFY(availableShaders.contains(QShaderKey(QShader::GlslShader, QShaderVersion(150))));
    QVERIFY(availableShaders.contains(QShaderKey(QShader::GlslShader, QShaderVersion(330))));

    const QShaderDescription desc = s.description();
    QVERIFY(desc.isValid());
    QCOMPARE(desc.inputVariables().count(), 1);
    for (const QShaderDescription::InOutVariable &v : desc.inputVariables()) {
        switch (v.location) {
        case 0:
            QCOMPARE(v.name, QByteArrayLiteral("qt_TexCoord"));
            QCOMPARE(v.type, QShaderDescription::Vec2);
            break;
        default:
            QFAIL(qPrintable(QStringLiteral("Bad location: %1").arg(v.location)));
            break;
        }
    }
    QCOMPARE(desc.outputVariables().count(), 1);
    for (const QShaderDescription::InOutVariable &v : desc.outputVariables()) {
        switch (v.location) {
        case 0:
            QCOMPARE(v.name, QByteArrayLiteral("fragColor"));
            QCOMPARE(v.type, QShaderDescription::Vec4);
            break;
        default:
            QFAIL(qPrintable(QStringLiteral("Bad location: %1").arg(v.location)));
            break;
        }
    }
    QCOMPARE(desc.uniformBlocks().count(), 1);
    const QShaderDescription::UniformBlock blk = desc.uniformBlocks().first();
    QCOMPARE(blk.blockName, QByteArrayLiteral("buf"));
    QCOMPARE(blk.structName, QByteArrayLiteral("ubuf"));
    QCOMPARE(blk.size, 68);
    QCOMPARE(blk.binding, 0);
    QCOMPARE(blk.descriptorSet, 0);
    QCOMPARE(blk.members.count(), 2);
    for (int i = 0; i < blk.members.count(); ++i) {
        const QShaderDescription::BlockVariable v = blk.members[i];
        switch (i) {
        case 0:
            QCOMPARE(v.offset, 0);
            QCOMPARE(v.size, 64);
            QCOMPARE(v.name, QByteArrayLiteral("qt_Matrix"));
            QCOMPARE(v.type, QShaderDescription::Mat4);
            QCOMPARE(v.matrixStride, 16);
            break;
        case 1:
            QCOMPARE(v.offset, 64);
            QCOMPARE(v.size, 4);
            QCOMPARE(v.name, QByteArrayLiteral("opacity"));
            QCOMPARE(v.type, QShaderDescription::Float);
            break;
        default:
            QFAIL(qPrintable(QStringLiteral("Bad many blocks: %1").arg(blk.members.count())));
            break;
        }
    }
}

void tst_QShader::manualShaderPackCreation()
{
    // Exercise manually building a QShader (instead of loading it from
    // serialized form). Some Qt modules may do this, in particular when OpenGL
    // and GLSL code that cannot be processed through the normal pipeline with
    // Vulkan SPIR-V as the primary target.

    static const char *FS =
        "#extension GL_OES_EGL_image_external : require\n"
        "varying vec2 v_texcoord;\n"
        "struct buf {\n"
        "    mat4 qt_Matrix;\n"
        "    float qt_Opacity;\n"
        "};\n"
        "uniform buf ubuf;\n"
        "uniform samplerExternalOES tex0;\n"
        "void main()\n"
        "{\n"
        "    gl_FragColor = ubuf.qt_Opacity * texture2D(tex0, v_texcoord);\n"
        "}\n";
    static const char *FS_GLES_PREAMBLE =
        "precision highp float;\n";
    // not necessarily sensible given the OES stuff but just for testing
    static const char *FS_GL_PREAMBLE =
        "#version 120\n";
    QByteArray fs_gles = FS_GLES_PREAMBLE;
    fs_gles += FS;
    QByteArray fs_gl = FS_GL_PREAMBLE;
    fs_gl += FS;

    QShaderDescription desc;
    QShaderDescriptionPrivate *descData = QShaderDescriptionPrivate::get(&desc);
    QCOMPARE(descData->ref.loadRelaxed(), 1);

    // Inputs
    QShaderDescription::InOutVariable texCoordInput;
    texCoordInput.name = "v_texcoord";
    texCoordInput.type = QShaderDescription::Vec2;
    texCoordInput.location = 0;

    descData->inVars = {
        texCoordInput
    };

    // Outputs (just here for completeness, not strictly needed with OpenGL, the
    // OpenGL backend of QRhi does not care)
    QShaderDescription::InOutVariable fragColorOutput;
    texCoordInput.name = "gl_FragColor";
    texCoordInput.type = QShaderDescription::Vec4;
    texCoordInput.location = 0;

    descData->outVars = {
        fragColorOutput
    };

    // No real uniform blocks in GLSL shaders used with QRhi, but metadata-wise
    // that's what the struct maps to in others shading languages.
    QShaderDescription::BlockVariable matrixBlockVar;
    matrixBlockVar.name = "qt_Matrix";
    matrixBlockVar.type = QShaderDescription::Mat4;
    matrixBlockVar.offset = 0;
    matrixBlockVar.size = 64;

    QShaderDescription::BlockVariable opacityBlockVar;
    opacityBlockVar.name = "qt_Opacity";
    opacityBlockVar.type = QShaderDescription::Float;
    opacityBlockVar.offset = 64;
    opacityBlockVar.size = 4;

    QShaderDescription::UniformBlock ubufStruct;
    ubufStruct.blockName = "buf";
    ubufStruct.structName = "ubuf";
    ubufStruct.size = 64 + 4;
    ubufStruct.binding = 0;
    ubufStruct.members = {
        matrixBlockVar,
        opacityBlockVar
    };

    descData->uniformBlocks = {
        ubufStruct
    };

    // Samplers
    QShaderDescription::InOutVariable samplerTex0;
    samplerTex0.name = "tex0";
    samplerTex0.type = QShaderDescription::SamplerExternalOES;
    // the struct with the "uniform block" content should be binding 0, samplers can then use 1, 2, ...
    samplerTex0.binding = 1;

    descData->combinedImageSamplers = {
        samplerTex0
    };

    // Now we have everything needed to construct a QShader suitable for OpenGL ES >=2.0 and OpenGL >=2.1
    QShader shaderPack;
    shaderPack.setStage(QShader::FragmentStage);
    shaderPack.setDescription(desc);
    shaderPack.setShader(QShaderKey(QShader::GlslShader, QShaderVersion(100, QShaderVersion::GlslEs)), QShaderCode(fs_gles));
    shaderPack.setShader(QShaderKey(QShader::GlslShader, QShaderVersion(120)), QShaderCode(fs_gl));

    // real world code would then pass the QShader to QSGMaterialShader::setShader() etc.

    const QByteArray serialized = shaderPack.serialized();
    QShader newShaderPack = QShader::fromSerialized(serialized);
    QCOMPARE(newShaderPack.availableShaders().count(), 2);
    QCOMPARE(newShaderPack.description().inputVariables().count(), 1);
    QCOMPARE(newShaderPack.description().outputVariables().count(), 1);
    QCOMPARE(newShaderPack.description().uniformBlocks().count(), 1);
    QCOMPARE(newShaderPack.description().combinedImageSamplers().count(), 1);
    QCOMPARE(newShaderPack.shader(QShaderKey(QShader::GlslShader, QShaderVersion(100, QShaderVersion::GlslEs))).shader(), fs_gles);
    QCOMPARE(newShaderPack.shader(QShaderKey(QShader::GlslShader, QShaderVersion(120))).shader(), fs_gl);
}

void tst_QShader::loadV6WithSeparateImagesAndSamplers()
{
    QShader s = getShader(QLatin1String(":/data/texture_sep_v6.frag.qsb"));
    QVERIFY(s.isValid());
    QCOMPARE(QShaderPrivate::get(&s)->qsbVersion, 6);

    const QList<QShaderKey> availableShaders = s.availableShaders();
    QCOMPARE(availableShaders.count(), 6);
    QVERIFY(availableShaders.contains(QShaderKey(QShader::SpirvShader, QShaderVersion(100))));
    QVERIFY(availableShaders.contains(QShaderKey(QShader::MslShader, QShaderVersion(12))));
    QVERIFY(availableShaders.contains(QShaderKey(QShader::HlslShader, QShaderVersion(50))));
    QVERIFY(availableShaders.contains(QShaderKey(QShader::GlslShader, QShaderVersion(100, QShaderVersion::GlslEs))));
    QVERIFY(availableShaders.contains(QShaderKey(QShader::GlslShader, QShaderVersion(120))));
    QVERIFY(availableShaders.contains(QShaderKey(QShader::GlslShader, QShaderVersion(150))));

    QShader::NativeResourceBindingMap resMap =
            s.nativeResourceBindingMap(QShaderKey(QShader::HlslShader, QShaderVersion(50)));
    QVERIFY(resMap.count() == 4);
    QVERIFY(s.separateToCombinedImageSamplerMappingList(QShaderKey(QShader::HlslShader, QShaderVersion(50))).isEmpty());
    resMap = s.nativeResourceBindingMap(QShaderKey(QShader::MslShader, QShaderVersion(12)));
    QVERIFY(resMap.count() == 4);
    QVERIFY(s.separateToCombinedImageSamplerMappingList(QShaderKey(QShader::MslShader, QShaderVersion(12))).isEmpty());

    for (auto key : {
         QShaderKey(QShader::GlslShader, QShaderVersion(100, QShaderVersion::GlslEs)),
         QShaderKey(QShader::GlslShader, QShaderVersion(120)),
         QShaderKey(QShader::GlslShader, QShaderVersion(150)) })
    {
         auto list = s.separateToCombinedImageSamplerMappingList(key);
         QCOMPARE(list.count(), 2);
    }
}

#include <tst_qshader.moc>
QTEST_MAIN(tst_QShader)
