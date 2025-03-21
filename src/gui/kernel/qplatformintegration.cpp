// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qplatformintegration.h"

#include <qpa/qplatformfontdatabase.h>
#include <qpa/qplatformclipboard.h>
#include <qpa/qplatformaccessibility.h>
#include <qpa/qplatformtheme.h>
#include <QtGui/private/qguiapplication_p.h>
#include <QtGui/private/qpixmap_raster_p.h>

#if QT_CONFIG(draganddrop)
#include <private/qdnd_p.h>
#include <private/qsimpledrag_p.h>
#endif

#ifndef QT_NO_SESSIONMANAGER
# include <qpa/qplatformsessionmanager.h>
#endif

QT_BEGIN_NAMESPACE

/*!
    Accessor for the platform integration's fontdatabase.

    Default implementation returns a default QPlatformFontDatabase.

    \sa QPlatformFontDatabase
*/
QPlatformFontDatabase *QPlatformIntegration::fontDatabase() const
{
    static QPlatformFontDatabase *db = nullptr;
    if (!db) {
        db = new QPlatformFontDatabase;
    }
    return db;
}

/*!
    Accessor for the platform integration's clipboard.

    Default implementation returns a default QPlatformClipboard.

    \sa QPlatformClipboard

*/

#ifndef QT_NO_CLIPBOARD

QPlatformClipboard *QPlatformIntegration::clipboard() const
{
    static QPlatformClipboard *clipboard = nullptr;
    if (!clipboard) {
        clipboard = new QPlatformClipboard;
    }
    return clipboard;
}

#endif

#if QT_CONFIG(draganddrop)
/*!
    Accessor for the platform integration's drag object.

    Default implementation returns QSimpleDrag. This class supports only drag
    and drop operations within the same Qt application.
*/
QPlatformDrag *QPlatformIntegration::drag() const
{
    static QSimpleDrag *drag = nullptr;
    if (!drag) {
        drag = new QSimpleDrag;
    }
    return drag;
}
#endif // QT_CONFIG(draganddrop)

QPlatformNativeInterface * QPlatformIntegration::nativeInterface() const
{
    return nullptr;
}

QPlatformServices *QPlatformIntegration::services() const
{
    return nullptr;
}

/*!
    \class QPlatformIntegration
    \since 4.8
    \internal
    \preliminary
    \ingroup qpa
    \brief The QPlatformIntegration class is the entry for WindowSystem specific functionality.

    QPlatformIntegration is the single entry point for windowsystem specific functionality when
    using the QPA platform. It has factory functions for creating platform specific pixmaps and
    windows. The class also controls the font subsystem.

    QPlatformIntegration is a singleton class which gets instantiated in the QGuiApplication
    constructor. The QPlatformIntegration instance do not have ownership of objects it creates in
    functions where the name starts with create. However, functions which don't have a name
    starting with create acts as accessors to member variables.

    It is not trivial to create or build a platform plugin outside of the Qt source tree. Therefore
    the recommended approach for making new platform plugin is to copy an existing plugin inside
    the QTSRCTREE/src/plugins/platform and develop the plugin inside the source tree.

    The minimal platform integration is the smallest platform integration it is possible to make,
    which makes it an ideal starting point for new plugins. For a slightly more advanced plugin,
    consider reviewing the directfb plugin, or the testlite plugin.
*/

/*!
    \fn QPlatformPixmap *QPlatformIntegration::createPlatformPixmap(QPlatformPixmap::PixelType type) const

    Factory function for QPlatformPixmap. PixelType can be either PixmapType or BitmapType.
    \sa QPlatformPixmap
*/

/*!
    \fn QPlatformWindow *QPlatformIntegration::createPlatformWindow(QWindow *window) const

    Factory function for QPlatformWindow. The \a window parameter is a pointer to the window
    which the QPlatformWindow is supposed to be created for.

    All windows have to have a QPlatformWindow, and it will be created on-demand when the
    QWindow is made visible for the first time, or explicitly through calling QWindow::create().

    In the constructor, of the QPlatformWindow, the window flags, state, title and geometry
    of the \a window should be applied to the underlying window. If the resulting flags or state
    differs, the resulting values should be set on the \a window using QWindow::setWindowFlags()
    or QWindow::setWindowState(), respectively.

    \sa QPlatformWindow, QPlatformWindowFormat
    \sa createPlatformBackingStore()
*/

/*!
    \fn QPlatformBackingStore *QPlatformIntegration::createPlatformBackingStore(QWindow *window) const

    Factory function for QPlatformBackingStore. The QWindow parameter is a pointer to the
    top level widget(tlw) the window surface is created for. A QPlatformWindow is always created
    before the QPlatformBackingStore for tlw where the widget also requires a backing store.

    \sa QBackingStore
    \sa createPlatformWindow()
*/

/*!
    \enum QPlatformIntegration::Capability

    Capabilities are used to determine specific features of a platform integration

    \value ThreadedPixmaps The platform uses a pixmap implementation that is reentrant
    and can be used from multiple threads, like the raster paint engine and QImage based
    pixmaps.

    \value OpenGL The platform supports OpenGL

    \value ThreadedOpenGL The platform supports using OpenGL outside the GUI thread.

    \value SharedGraphicsCache The platform supports a shared graphics cache

    \value BufferQueueingOpenGL Deprecated. The OpenGL implementation on the platform will
    queue up buffers when swapBuffers() is called and block only when its buffer pipeline
    is full, rather than block immediately.

    \value MultipleWindows The platform supports multiple QWindows, i.e. does some kind
    of compositing either client or server side. Some platforms might only support a
    single fullscreen window.

    \value ApplicationState The platform handles the application state explicitly.
    This means that QEvent::ApplicationActivate and QEvent::ApplicationDeativate
    will not be posted automatically. Instead, the platform must handle application
    state explicitly by using QWindowSystemInterface::handleApplicationStateChanged().
    If not set, application state will follow window activation, which is the normal
    behavior for desktop platforms.

    \value ForeignWindows The platform allows creating QWindows which represent
    native windows created by other processes or by using native libraries.

    \value NonFullScreenWindows The platform supports top-level windows which do not
    fill the screen. The default implementation returns \c true. Returning false for
    this will cause all windows, including dialogs and popups, to be resized to fill the
    screen.

    \value WindowManagement The platform is based on a system that performs window
    management.  This includes the typical desktop platforms. Can be set to false on
    platforms where no window management is available, meaning for example that windows
    are never repositioned by the window manager. The default implementation returns \c true.

    \value AllGLFunctionsQueryable Deprecated. Used to indicate whether the QOpenGLContext
    backend provided by the platform is
    able to return function pointers from getProcAddress() even for standard OpenGL
    functions, for example OpenGL 1 functions like glClear() or glDrawArrays(). This is
    important because the OpenGL specifications do not require this ability from the
    getProcAddress implementations of the windowing system interfaces (EGL, WGL, GLX). The
    platform plugins may however choose to enhance the behavior in the backend
    implementation for QOpenGLContext::getProcAddress() and support returning a function
    pointer also for the standard, non-extension functions. This capability is a
    prerequisite for dynamic OpenGL loading. Starting with Qt 5.7, the platform plugin
    is required to have this capability.

    \value ApplicationIcon The platform supports setting the application icon. (since 5.5)

    \value TopStackedNativeChildWindows The platform supports native child windows via
    QWindowContainer without having to punch a transparent hole in the
    backingstore. (since 5.10)

    \value OpenGLOnRasterSurface The platform supports making a QOpenGLContext current
    in combination with a QWindow of type RasterSurface.

    \value PaintEvents The platform sends paint events instead of expose events when
    the window needs repainting. Expose events are only sent when a window is toggled
    from a non-exposed to exposed state or back.

    \value RhiBasedRendering The platform supports one or more of the 3D rendering APIs
    that Qt Quick and other components can use via the Qt Rendering Hardware Interface. On
    platforms where it is clear upfront that the platform cannot, or does not want to,
    support rendering via 3D graphics APIs such as OpenGL, Vulkan, Direct 3D, or Metal,
    this capability can be reported as \c false. That in effect means that in modules
    where there is an alternative, such as Qt Quick with its \c software backend, an
    automatic fallback to that alternative may occur, if applicable. The default
    implementation of hasCapability() returns \c true.

    \value ScreenWindowGrabbing The platform supports grabbing window on screen.
    On Wayland, this capability can be reported as \c false. The default implementation
    of hasCapability() returns \c true.
 */

/*!

    \fn QAbstractEventDispatcher *QPlatformIntegration::createEventDispatcher() const = 0

    Factory function for the GUI event dispatcher. The platform plugin should create
    and return a QAbstractEventDispatcher subclass when this function is called.

    If the platform plugin for some reason creates the event dispatcher outside of
    this function (for example in the constructor), it needs to handle the case
    where this function is never called, ensuring that the event dispatcher is
    still deleted at some point (typically in the destructor).

    Note that the platform plugin should never explicitly set the event dispatcher
    itself, using QCoreApplication::setEventDispatcher(), but let QCoreApplication
    decide when and which event dispatcher to create.

    \since 5.2
*/

bool QPlatformIntegration::hasCapability(Capability cap) const
{
    return cap == NonFullScreenWindows || cap == NativeWidgets || cap == WindowManagement
        || cap == TopStackedNativeChildWindows || cap == WindowActivation
        || cap == RhiBasedRendering || cap == ScreenWindowGrabbing;
}

QPlatformPixmap *QPlatformIntegration::createPlatformPixmap(QPlatformPixmap::PixelType type) const
{
    return new QRasterPlatformPixmap(type);
}

#ifndef QT_NO_OPENGL
/*!
    Factory function for QPlatformOpenGLContext. The \a context parameter is a pointer to
    the context for which a platform-specific context backend needs to be
    created. Configuration settings like the format, share context and screen have to be
    taken from this QOpenGLContext and the resulting platform context is expected to be
    backed by a native context that fulfills these criteria.

    If the context has native handles set, no new native context is expected to be created.
    Instead, the provided handles have to be used. In this case the ownership of the handle
    must not be taken and the platform implementation is not allowed to destroy the native
    context. Configuration parameters like the format are also to be ignored. Instead, the
    platform implementation is responsible for querying the configuriation from the provided
    native context.

    Returns a pointer to a QPlatformOpenGLContext instance or \nullptr if the context could
    not be created.

    \sa QOpenGLContext
*/
QPlatformOpenGLContext *QPlatformIntegration::createPlatformOpenGLContext(QOpenGLContext *context) const
{
    Q_UNUSED(context);
    qWarning("This plugin does not support createPlatformOpenGLContext!");
    return nullptr;
}
#endif // QT_NO_OPENGL

/*!
   Factory function for QPlatformSharedGraphicsCache. This function will return 0 if the platform
   integration does not support any shared graphics cache mechanism for the given \a cacheId.
*/
QPlatformSharedGraphicsCache *QPlatformIntegration::createPlatformSharedGraphicsCache(const char *cacheId) const
{
    qWarning("This plugin does not support createPlatformSharedGraphicsBuffer for cacheId: %s!",
             cacheId);
    return nullptr;
}

/*!
   Factory function for QPaintEngine. This function will return 0 if the platform
   integration does not support creating any paint engine the given \a paintDevice.
*/
QPaintEngine *QPlatformIntegration::createImagePaintEngine(QPaintDevice *paintDevice) const
{
    Q_UNUSED(paintDevice);
    return nullptr;
}

/*!
  Performs initialization steps that depend on having an event dispatcher
  available. Called after the event dispatcher has been created.

  Tasks that require an event dispatcher, for example creating socket notifiers, cannot be
  performed in the constructor. Instead, they should be performed here. The default
  implementation does nothing.
*/
void QPlatformIntegration::initialize()
{
}

/*!
  Called before the platform integration is deleted. Useful when cleanup relies on virtual
  functions.

  \since 5.5
*/
void QPlatformIntegration::destroy()
{
}

/*!
  Returns the platforms input context.

  The default implementation returns \nullptr, implying no input method support.
*/
QPlatformInputContext *QPlatformIntegration::inputContext() const
{
    return nullptr;
}

#if QT_CONFIG(accessibility)

/*!
  Returns the platforms accessibility.

  The default implementation returns QPlatformAccessibility which
  delegates handling of accessibility to accessiblebridge plugins.
*/
QPlatformAccessibility *QPlatformIntegration::accessibility() const
{
    static QPlatformAccessibility *accessibility = nullptr;
    if (Q_UNLIKELY(!accessibility)) {
        accessibility = new QPlatformAccessibility;
    }
    return accessibility;
}

#endif

QVariant QPlatformIntegration::styleHint(StyleHint hint) const
{
    switch (hint) {
    case CursorFlashTime:
        return QPlatformTheme::defaultThemeHint(QPlatformTheme::CursorFlashTime);
    case KeyboardInputInterval:
        return QPlatformTheme::defaultThemeHint(QPlatformTheme::KeyboardInputInterval);
    case KeyboardAutoRepeatRate:
        return QPlatformTheme::defaultThemeHint(QPlatformTheme::KeyboardAutoRepeatRate);
    case MouseDoubleClickInterval:
        return QPlatformTheme::defaultThemeHint(QPlatformTheme::MouseDoubleClickInterval);
    case StartDragDistance:
        return QPlatformTheme::defaultThemeHint(QPlatformTheme::StartDragDistance);
    case StartDragTime:
        return QPlatformTheme::defaultThemeHint(QPlatformTheme::StartDragTime);
    case ShowIsFullScreen:
        return false;
    case ShowIsMaximized:
        return false;
    case ShowShortcutsInContextMenus:
        return QPlatformTheme::defaultThemeHint(QPlatformTheme::ShowShortcutsInContextMenus);
    case PasswordMaskDelay:
        return QPlatformTheme::defaultThemeHint(QPlatformTheme::PasswordMaskDelay);
    case PasswordMaskCharacter:
        return QPlatformTheme::defaultThemeHint(QPlatformTheme::PasswordMaskCharacter);
    case FontSmoothingGamma:
        return qreal(1.7);
    case StartDragVelocity:
        return QPlatformTheme::defaultThemeHint(QPlatformTheme::StartDragVelocity);
    case UseRtlExtensions:
        return QVariant(false);
    case SetFocusOnTouchRelease:
        return QVariant(false);
    case MousePressAndHoldInterval:
        return QPlatformTheme::defaultThemeHint(QPlatformTheme::MousePressAndHoldInterval);
    case TabFocusBehavior:
        return QPlatformTheme::defaultThemeHint(QPlatformTheme::TabFocusBehavior);
    case ReplayMousePressOutsidePopup:
        return true;
    case ItemViewActivateItemOnSingleClick:
        return QPlatformTheme::defaultThemeHint(QPlatformTheme::ItemViewActivateItemOnSingleClick);
    case UiEffects:
        return QPlatformTheme::defaultThemeHint(QPlatformTheme::UiEffects);
    case WheelScrollLines:
        return QPlatformTheme::defaultThemeHint(QPlatformTheme::WheelScrollLines);
    case MouseQuickSelectionThreshold:
        return QPlatformTheme::defaultThemeHint(QPlatformTheme::MouseQuickSelectionThreshold);
    case MouseDoubleClickDistance:
        return QPlatformTheme::defaultThemeHint(QPlatformTheme::MouseDoubleClickDistance);
    }

    return 0;
}

Qt::WindowState QPlatformIntegration::defaultWindowState(Qt::WindowFlags flags) const
{
    // Leave popup-windows as is
    if (flags & Qt::Popup & ~Qt::Window)
        return Qt::WindowNoState;

     if (flags & Qt::SubWindow)
        return Qt::WindowNoState;

    if (styleHint(QPlatformIntegration::ShowIsFullScreen).toBool())
        return Qt::WindowFullScreen;
    else if (styleHint(QPlatformIntegration::ShowIsMaximized).toBool())
        return Qt::WindowMaximized;

    return Qt::WindowNoState;
}

Qt::KeyboardModifiers QPlatformIntegration::queryKeyboardModifiers() const
{
    return QGuiApplication::keyboardModifiers();
}

/*!
  Should be used to obtain a list of possible shortcuts for the given key
  event. Shortcuts should be encoded as int(Qt::Key + Qt::KeyboardModifiers).

  One example for more than one possibility is the key combination of Shift+5.
  That one might trigger a shortcut which is set as "Shift+5" as well as one
  using %. These combinations depend on the currently set keyboard layout.

  \note This function should be called only from key event handlers.
*/
QList<int> QPlatformIntegration::possibleKeys(const QKeyEvent *) const
{
    return QList<int>();
}

QStringList QPlatformIntegration::themeNames() const
{
    return QStringList();
}

class QPlatformTheme *QPlatformIntegration::createPlatformTheme(const QString &name) const
{
    Q_UNUSED(name);
    return new QPlatformTheme;
}

/*!
   Factory function for QOffscreenSurface. An offscreen surface will typically be implemented with a
   pixel buffer (pbuffer). If the platform doesn't support offscreen surfaces, an invisible window
   will be used by QOffscreenSurface instead.
*/
QPlatformOffscreenSurface *QPlatformIntegration::createPlatformOffscreenSurface(QOffscreenSurface *surface) const
{
    Q_UNUSED(surface);
    return nullptr;
}

#ifndef QT_NO_SESSIONMANAGER
/*!
   \since 5.2

   Factory function for QPlatformSessionManager. The default QPlatformSessionManager provides the same
   functionality as the QSessionManager.
*/
QPlatformSessionManager *QPlatformIntegration::createPlatformSessionManager(const QString &id, const QString &key) const
{
    return new QPlatformSessionManager(id, key);
}
#endif

/*!
   \since 5.2

   Function to sync the platform integrations state with the window system.

   This is often implemented as a roundtrip from the platformintegration to the window system.

   This function should not call QWindowSystemInterface::flushWindowSystemEvents() or
   QCoreApplication::processEvents()
*/
void QPlatformIntegration::sync()
{
}

/*!
   \since 5.7

    Should sound a bell, using the default volume and sound.

    \sa QApplication::beep()
*/
void QPlatformIntegration::beep() const
{
}

/*!
   \since 6.0

   Asks the platform to terminate the application.

   Overrides should ensure there's a callback into the QWSI
   function handleApplicationTermination so that the quit can
   be propagated to QtGui and the application.
*/
void QPlatformIntegration::quit() const
{
    QWindowSystemInterface::handleApplicationTermination<QWindowSystemInterface::SynchronousDelivery>();
}

#ifndef QT_NO_OPENGL
/*!
  Platform integration function for querying the OpenGL implementation type.

  Used only when dynamic OpenGL implementation loading is enabled.

  Subclasses should reimplement this function and return a value based on
  the OpenGL implementation they have chosen to load.

  \note The return value does not indicate or limit the types of
  contexts that can be created by a given implementation. For example
  a desktop OpenGL implementation may be capable of creating OpenGL
  ES-compatible contexts too.

  \sa QOpenGLContext::openGLModuleType(), QOpenGLContext::isOpenGLES()

  \since 5.3
 */
QOpenGLContext::OpenGLModuleType QPlatformIntegration::openGLModuleType()
{
    qWarning("This plugin does not support dynamic OpenGL loading!");
    return QOpenGLContext::LibGL;
}
#endif

/*!
    \since 5.5

    Platform integration function for setting the application icon.

    \sa QGuiApplication::setWindowIcon()
*/
void QPlatformIntegration::setApplicationIcon(const QIcon &icon) const
{
    Q_UNUSED(icon);
}

#if QT_CONFIG(vulkan) || defined(Q_CLANG_QDOC)

/*!
    Factory function for QPlatformVulkanInstance. The \a instance parameter is a
    pointer to the instance for which a platform-specific backend needs to be
    created.

    Returns a pointer to a QPlatformOpenGLContext instance or \nullptr if the context could
    not be created.

    \sa QVulkanInstance
    \since 5.10
*/
QPlatformVulkanInstance *QPlatformIntegration::createPlatformVulkanInstance(QVulkanInstance *instance) const
{
    Q_UNUSED(instance);
    qWarning("This plugin does not support createPlatformVulkanInstance");
    return nullptr;
}

#endif // QT_CONFIG(vulkan)

QT_END_NAMESPACE
