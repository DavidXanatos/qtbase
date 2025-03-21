// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

//! [0]
label->setAlignment(Qt::AlignLeft | Qt::AlignTop);
label->setAlignment({ });
//! [0]


//! [1]
class MyClass
{
public:
    enum Option {
        NoOptions = 0x0,
        ShowTabs = 0x1,
        ShowAll = 0x2,
        SqueezeBlank = 0x4
    };
    Q_DECLARE_FLAGS(Options, Option)
    ...
};

Q_DECLARE_OPERATORS_FOR_FLAGS(MyClass::Options)
//! [1]

//! [meta-object flags]
Q_FLAG(Options)
//! [meta-object flags]

//! [2]
typedef QFlags<Enum> Flags;
//! [2]


//! [3]
int myValue = 10;
int minValue = 2;
int maxValue = 6;

int boundedValue = qBound(minValue, myValue, maxValue);
// boundedValue == 6
//! [3]


//! [4]
if (!driver()->isOpen() || driver()->isOpenError()) {
    qWarning("QSqlQuery::exec: database not open");
    return false;
}
//! [4]


//! [5]
qint64 value = Q_INT64_C(932838457459459);
//! [5]


//! [6]
quint64 value = Q_UINT64_C(932838457459459);
//! [6]


//! [7]
void myMsgHandler(QtMsgType, const char *);
//! [7]


//! [8]
qint64 value = Q_INT64_C(932838457459459);
//! [8]


//! [9]
quint64 value = Q_UINT64_C(932838457459459);
//! [9]


//! [10]
int absoluteValue;
int myValue = -4;

absoluteValue = qAbs(myValue);
// absoluteValue == 4
//! [10]


//! [11A]
double valueA = 2.3;
double valueB = 2.7;

int roundedValueA = qRound(valueA);
// roundedValueA = 2
int roundedValueB = qRound(valueB);
// roundedValueB = 3
//! [11A]

//! [11B]
float valueA = 2.3;
float valueB = 2.7;

int roundedValueA = qRound(valueA);
// roundedValueA = 2
int roundedValueB = qRound(valueB);
// roundedValueB = 3
//! [11B]


//! [12A]
double valueA = 42949672960.3;
double valueB = 42949672960.7;

qint64 roundedValueA = qRound64(valueA);
// roundedValueA = 42949672960
qint64 roundedValueB = qRound64(valueB);
// roundedValueB = 42949672961
//! [12A]

//! [12B]
float valueA = 42949672960.3;
float valueB = 42949672960.7;

qint64 roundedValueA = qRound64(valueA);
// roundedValueA = 42949672960
qint64 roundedValueB = qRound64(valueB);
// roundedValueB = 42949672961
//! [12B]


//! [13]
int myValue = 6;
int yourValue = 4;

int minValue = qMin(myValue, yourValue);
// minValue == yourValue
//! [13]


//! [14]
int myValue = 6;
int yourValue = 4;

int maxValue = qMax(myValue, yourValue);
// maxValue == myValue
//! [14]


//! [15]
int myValue = 10;
int minValue = 2;
int maxValue = 6;

int boundedValue = qBound(minValue, myValue, maxValue);
// boundedValue == 6
//! [15]


//! [16]
#if QT_VERSION >= QT_VERSION_CHECK(4, 1, 0)
    QIcon icon = style()->standardIcon(QStyle::SP_TrashIcon);
#else
    QPixmap pixmap = style()->standardPixmap(QStyle::SP_TrashIcon);
    QIcon icon(pixmap);
#endif
//! [16]


//! [17]
// File: div.cpp

#include <QtGlobal>

int divide(int a, int b)
{
    Q_ASSERT(b != 0);
    return a / b;
}
//! [17]


//! [18]
ASSERT: "b != 0" in file div.cpp, line 7
//! [18]


//! [19]
// File: div.cpp

#include <QtGlobal>

int divide(int a, int b)
{
    Q_ASSERT_X(b != 0, "divide", "division by zero");
    return a / b;
}
//! [19]


//! [20]
ASSERT failure in divide: "division by zero", file div.cpp, line 7
//! [20]


//! [21]
int *a;

Q_CHECK_PTR(a = new int[80]);   // WRONG!

a = new (nothrow) int[80];      // Right
Q_CHECK_PTR(a);
//! [21]


//! [22]
template<typename TInputType>
const TInputType &myMin(const TInputType &value1, const TInputType &value2)
{
    qDebug() << Q_FUNC_INFO << "was called with value1:" << value1 << "value2:" << value2;

    if(value1 < value2)
        return value1;
    else
        return value2;
}
//! [22]


//! [23]
#include <qapplication.h>
#include <stdio.h>
#include <stdlib.h>

void myMessageOutput(QtMsgType type, const QMessageLogContext &context, const QString &msg)
{
    QByteArray localMsg = msg.toLocal8Bit();
    const char *file = context.file ? context.file : "";
    const char *function = context.function ? context.function : "";
    switch (type) {
    case QtDebugMsg:
        fprintf(stderr, "Debug: %s (%s:%u, %s)\n", localMsg.constData(), file, context.line, function);
        break;
    case QtInfoMsg:
        fprintf(stderr, "Info: %s (%s:%u, %s)\n", localMsg.constData(), file, context.line, function);
        break;
    case QtWarningMsg:
        fprintf(stderr, "Warning: %s (%s:%u, %s)\n", localMsg.constData(), file, context.line, function);
        break;
    case QtCriticalMsg:
        fprintf(stderr, "Critical: %s (%s:%u, %s)\n", localMsg.constData(), file, context.line, function);
        break;
    case QtFatalMsg:
        fprintf(stderr, "Fatal: %s (%s:%u, %s)\n", localMsg.constData(), file, context.line, function);
        break;
    }
}

int main(int argc, char **argv)
{
    qInstallMessageHandler(myMessageOutput);
    QApplication app(argc, argv);
    ...
    return app.exec();
}
//! [23]


//! [24]
qDebug("Items in list: %d", myList.size());
//! [24]


//! [25]
qDebug() << "Brush:" << myQBrush << "Other value:" << i;
//! [25]


//! [qInfo_printf]
qInfo("Items in list: %d", myList.size());
//! [qInfo_printf]

//! [qInfo_stream]
qInfo() << "Brush:" << myQBrush << "Other value:" << i;
//! [qInfo_stream]

//! [26]
void f(int c)
{
    if (c > 200)
        qWarning("f: bad argument, c == %d", c);
}
//! [26]


//! [27]
qWarning() << "Brush:" << myQBrush << "Other value:" << i;
//! [27]


//! [28]
void load(const QString &fileName)
{
    QFile file(fileName);
    if (!file.exists())
        qCritical("File '%s' does not exist!", qUtf8Printable(fileName));
}
//! [28]


//! [29]
qCritical() << "Brush:" << myQBrush << "Other value:" << i;
//! [29]


//! [30]
int divide(int a, int b)
{
    if (b == 0)                                // program error
        qFatal("divide: cannot divide by zero");
    return a / b;
}
//! [30]


//! [31]
forever {
    ...
}
//! [31]


//! [32]
CONFIG += no_keywords
//! [32]


//! [33]
CONFIG += no_keywords
//! [33]


//! [34]
QString FriendlyConversation::greeting(int type)
{
    static const char *greeting_strings[] = {
        QT_TR_NOOP("Hello"),
        QT_TR_NOOP("Goodbye")
    };
    return tr(greeting_strings[type]);
}
//! [34]


//! [35]
static const char *greeting_strings[] = {
    QT_TRANSLATE_NOOP("FriendlyConversation", "Hello"),
    QT_TRANSLATE_NOOP("FriendlyConversation", "Goodbye")
};

QString FriendlyConversation::greeting(int type)
{
    return tr(greeting_strings[type]);
}

QString global_greeting(int type)
{
    return qApp->translate("FriendlyConversation",
                           greeting_strings[type]);
}
//! [35]


//! [36]

static { const char *source; const char *comment; } greeting_strings[] =
{
    QT_TRANSLATE_NOOP3("FriendlyConversation", "Hello",
                       "A really friendly hello"),
    QT_TRANSLATE_NOOP3("FriendlyConversation", "Goodbye",
                       "A really friendly goodbye")
};

QString FriendlyConversation::greeting(int type)
{
    return tr(greeting_strings[type].source,
              greeting_strings[type].comment);
}

QString global_greeting(int type)
{
    return qApp->translate("FriendlyConversation",
                           greeting_strings[type].source,
                           greeting_strings[type].comment);
}
//! [36]


//! [qttrnnoop]
static const char * const StatusClass::status_strings[] = {
    QT_TR_N_NOOP("There are %n new message(s)"),
    QT_TR_N_NOOP("There are %n total message(s)")
};

QString StatusClass::status(int type, int count)
{
    return tr(status_strings[type], nullptr, count);
}
//! [qttrnnoop]

//! [qttranslatennoop]
static const char * const greeting_strings[] = {
    QT_TRANSLATE_N_NOOP("Welcome Msg", "Hello, you have %n message(s)"),
    QT_TRANSLATE_N_NOOP("Welcome Msg", "Hi, you have %n message(s)")
};

QString global_greeting(int type, int msgcnt)
{
    return translate("Welcome Msg", greeting_strings[type], nullptr, msgcnt);
}
//! [qttranslatennoop]

//! [qttranslatennoop3]
static { const char * const source; const char * const comment; } status_strings[] = {
    QT_TRANSLATE_N_NOOP3("Message Status", "Hello, you have %n message(s)",
                         "A login message status"),
    QT_TRANSLATE_N_NOOP3("Message status", "You have %n new message(s)",
                         "A new message query status")
};

QString FriendlyConversation::greeting(int type, int count)
{
    return tr(status_strings[type].source,
              status_strings[type].comment, count);
}

QString global_greeting(int type, int count)
{
    return qApp->translate("Message Status",
                           status_strings[type].source,
                           status_strings[type].comment,
                           count);
}
//! [qttranslatennoop3]


//! [qttrid]
    //% "%n fooish bar(s) found.\n"
    //% "Do you want to continue?"
    QString text = qtTrId("qtn_foo_bar", n);
//! [qttrid]


//! [qttrid_noop]
static const char * const ids[] = {
    //% "This is the first text."
    QT_TRID_NOOP("qtn_1st_text"),
    //% "This is the second text."
    QT_TRID_NOOP("qtn_2nd_text"),
    0
};

void TheClass::addLabels()
{
    for (int i = 0; ids[i]; ++i)
        new QLabel(qtTrId(ids[i]), this);
}
//! [qttrid_noop]

//! [qttrid_n_noop]
static const char * const ids[] = {
    //% "%n foo(s) found."
    QT_TRID_N_NOOP("qtn_foo"),
    //% "%n bar(s) found."
    QT_TRID_N_NOOP("qtn_bar"),
    0
};

QString result(int type, int n)
{
    return qtTrId(ids[type], n);
}
//! [qttrid_n_noop]

//! [37]
qWarning("%s: %s", qUtf8Printable(key), qUtf8Printable(value));
//! [37]


//! [qUtf16Printable]
qWarning("%ls: %ls", qUtf16Printable(key), qUtf16Printable(value));
//! [qUtf16Printable]


//! [38]
struct Point2D
{
    int x;
    int y;
};

Q_DECLARE_TYPEINFO(Point2D, Q_PRIMITIVE_TYPE);
//! [38]


//! [39]
class Point2D
{
public:
    Point2D() { data = new int[2]; }
    Point2D(const Point2D &other) { ... }
    ~Point2D() { delete[] data; }

    Point2D &operator=(const Point2D &other) { ... }

    int x() const { return data[0]; }
    int y() const { return data[1]; }

private:
    int *data;
};

Q_DECLARE_TYPEINFO(Point2D, Q_RELOCATABLE_TYPE);
//! [39]


//! [40]
#if Q_BYTE_ORDER == Q_BIG_ENDIAN
...
#endif

or

#if Q_BYTE_ORDER == Q_LITTLE_ENDIAN
...
#endif

//! [40]


//! [41]

#if Q_BYTE_ORDER == Q_LITTLE_ENDIAN
...
#endif

//! [41]


//! [42]
#if Q_BYTE_ORDER == Q_BIG_ENDIAN
...
#endif

//! [42]

//! [begin namespace macro]
namespace QT_NAMESPACE {
//! [begin namespace macro]

//! [end namespace macro]
}
//! [end namespace macro]

//! [43]
class MyClass : public QObject
{
private:
    Q_DISABLE_COPY(MyClass)
};

//! [43]

//! [44]
class MyClass : public QObject
{
private:
    MyClass(const MyClass &) = delete;
    MyClass &operator=(const MyClass &) = delete;
};
//! [44]

//! [45]
QWidget w = QWidget();
//! [45]

//! [46]
// Instead of comparing with 0.0
qFuzzyCompare(0.0, 1.0e-200); // This will return false
// Compare adding 1 to both values will fix the problem
qFuzzyCompare(1 + 0.0, 1 + 1.0e-200); // This will return true
//! [46]

//! [47]
CApaApplication *myApplicationFactory();
//! [47]


//! [49]
void myMessageHandler(QtMsgType, const QMessageLogContext &, const QString &);
//! [49]

//! [50]
class B {...};
class C {...};
class D {...};
struct A : public B {
    C c;
    D d;
};
//! [50]

//! [51]
template<> class QTypeInfo<A> : public QTypeInfoMerger<A, B, C, D> {};
//! [51]

//! [52]
    struct Foo {
        void overloadedFunction();
        void overloadedFunction(int, const QString &);
    };
    ... qOverload<>(&Foo::overloadedFunction)
    ... qOverload<int, const QString &>(&Foo::overloadedFunction)
//! [52]

//! [53]
    ... QOverload<>::of(&Foo::overloadedFunction)
    ... QOverload<int, const QString &>::of(&Foo::overloadedFunction)
//! [53]

//! [54]
    struct Foo {
        void overloadedFunction(int, const QString &);
        void overloadedFunction(int, const QString &) const;
    };
    ... qConstOverload<int, const QString &>(&Foo::overloadedFunction)
    ... qNonConstOverload<int, const QString &>(&Foo::overloadedFunction)
//! [54]

//! [qlikely]
    // the condition inside the "if" will be successful most of the times
    for (int i = 1; i <= 365; i++) {
        if (Q_LIKELY(isWorkingDay(i))) {
            ...
        }
        ...
    }
//! [qlikely]

//! [qunlikely]
bool readConfiguration(const QFile &file)
{
    // We expect to be asked to read an existing file
    if (Q_UNLIKELY(!file.exists())) {
        qWarning() << "File not found";
        return false;
    }

    ...
    return true;
}
//! [qunlikely]

//! [qunreachable-enum]
   enum Shapes {
       Rectangle,
       Triangle,
       Circle,
       NumShapes
   };
//! [qunreachable-enum]

//! [qunreachable-switch]
   switch (shape) {
       case Rectangle:
           return rectangle();
       case Triangle:
           return triangle();
       case Circle:
           return circle();
       case NumShapes:
           Q_UNREACHABLE();
           break;
   }
//! [qunreachable-switch]

//! [qt-version-check]
#include <QtGlobal>

#if QT_VERSION >= QT_VERSION_CHECK(5, 0, 0)
#include <QtWidgets>
#else
#include <QtGui>
#endif
//! [qt-version-check]

//! [is-empty]
    qgetenv(varName).isEmpty()
//! [is-empty]

//! [to-int]
    qgetenv(varName).toInt(ok, 0)
//! [to-int]

//! [is-null]
    !qgetenv(varName).isNull()
//! [is-null]

//! [as-const-0]
    QString s = ...;
    for (QChar ch : s) // detaches 's' (performs a deep-copy if 's' was shared)
        process(ch);
    for (QChar ch : qAsConst(s)) // ok, no detach attempt
        process(ch);
//! [as-const-0]

//! [as-const-1]
    const QString s = ...;
    for (QChar ch : s) // ok, no detach attempt on const objects
        process(ch);
//! [as-const-1]

//! [as-const-2]
    for (QChar ch : funcReturningQString())
        process(ch); // OK, the returned object is kept alive for the loop's duration
//! [as-const-2]

//! [as-const-3]
    for (QChar ch : qAsConst(funcReturningQString()))
        process(ch); // ERROR: ch is copied from deleted memory
//! [as-const-3]

//! [as-const-4]
    for (QChar ch : qAsConst(funcReturningQString()))
        process(ch); // ERROR: ch is copied from deleted memory
//! [as-const-4]

//! [qterminate]
    try { expr; } catch(...) { qTerminate(); }
//! [qterminate]

//! [qdecloverride]
    // generate error if this doesn't actually override anything:
    virtual void MyWidget::paintEvent(QPaintEvent*) override;
//! [qdecloverride]

//! [qdeclfinal-1]
    // more-derived classes no longer permitted to override this:
    virtual void MyWidget::paintEvent(QPaintEvent*) final;
//! [qdeclfinal-1]

//! [qdeclfinal-2]
    class QRect final { // cannot be derived from
        // ...
    };
//! [qdeclfinal-2]
