// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include <QTest>
#include <QDebug>
#include <QSignalSpy>
#include <QTimer>

#include <QtNetwork/QSslServer>
#include <QtNetwork/QSslKey>
#include "private/qtlsbackend_p.h"

class tst_QSslServer : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase();
    void testOneSuccessfulConnection();
    void testSelfSignedCertificateRejectedByServer();
    void testSelfSignedCertificateRejectedByClient();
#if QT_CONFIG(openssl)
    void testHandshakeInterruptedOnError();
    void testPreSharedKeyAuthenticationRequired();
#endif

private:
    QString testDataDir;
    bool isTestingOpenSsl = false;
    QSslConfiguration selfSignedClientQSslConfiguration();
    QSslConfiguration selfSignedServerQSslConfiguration();
    QSslConfiguration createQSslConfiguration(QString keyFileName, QString certificateFileName);
};

class SslServerSpy : public QObject
{
    Q_OBJECT

public:
    SslServerSpy(QSslConfiguration &configuration);

    QSslServer server;
    QSignalSpy sslErrorsSpy;
    QSignalSpy peerVerifyErrorSpy;
    QSignalSpy errorOccurredSpy;
    QSignalSpy pendingConnectionAvailableSpy;
    QSignalSpy preSharedKeyAuthenticationRequiredSpy;
    QSignalSpy alertSentSpy;
    QSignalSpy alertReceivedSpy;
    QSignalSpy handshakeInterruptedOnErrorSpy;
    QSignalSpy startedEncryptionHandshakeSpy;
};

SslServerSpy::SslServerSpy(QSslConfiguration &configuration)
    : server(),
      sslErrorsSpy(&server, &QSslServer::sslErrors),
      peerVerifyErrorSpy(&server, &QSslServer::peerVerifyError),
      errorOccurredSpy(&server, &QSslServer::errorOccurred),
      pendingConnectionAvailableSpy(&server, &QSslServer::pendingConnectionAvailable),
      preSharedKeyAuthenticationRequiredSpy(&server,
                                            &QSslServer::preSharedKeyAuthenticationRequired),
      alertSentSpy(&server, &QSslServer::alertSent),
      alertReceivedSpy(&server, &QSslServer::alertReceived),
      handshakeInterruptedOnErrorSpy(&server, &QSslServer::handshakeInterruptedOnError),
      startedEncryptionHandshakeSpy(&server, &QSslServer::startedEncryptionHandshake)
{
    server.setSslConfiguration(configuration);
}

void tst_QSslServer::initTestCase()
{
    testDataDir = QFileInfo(QFINDTESTDATA("certs")).absolutePath();
    if (testDataDir.isEmpty())
        testDataDir = QCoreApplication::applicationDirPath();
    if (!testDataDir.endsWith(QLatin1String("/")))
        testDataDir += QLatin1String("/");

    const QString openSslBackend = QTlsBackend::builtinBackendNames[QTlsBackend::nameIndexOpenSSL];
    const auto &tlsBackends = QSslSocket::availableBackends();
    if (tlsBackends.contains(openSslBackend)) {
        isTestingOpenSsl = true;
    }
}

QSslConfiguration tst_QSslServer::selfSignedClientQSslConfiguration()
{
    return createQSslConfiguration(testDataDir + "certs/selfsigned-client.key",
                                   testDataDir + "certs/selfsigned-client.crt");
}

QSslConfiguration tst_QSslServer::selfSignedServerQSslConfiguration()
{
    return createQSslConfiguration(testDataDir + "certs/selfsigned-server.key",
                                   testDataDir + "certs/selfsigned-server.crt");
}

QSslConfiguration tst_QSslServer::createQSslConfiguration(QString keyFileName,
                                                          QString certificateFileName)
{
    QSslConfiguration configuration(QSslConfiguration::defaultConfiguration());

    QFile keyFile(keyFileName);
    if (keyFile.open(QIODevice::ReadOnly)) {
        QSslKey key(keyFile.readAll(), QSsl::Rsa, QSsl::Pem, QSsl::PrivateKey);
        if (!key.isNull()) {
            configuration.setPrivateKey(key);
        } else {
            qCritical() << "Could not parse key: " << keyFileName;
        }
    } else {
        qCritical() << "Could not find key: " << keyFileName;
    }

    QList<QSslCertificate> localCert = QSslCertificate::fromPath(certificateFileName);
    if (!localCert.isEmpty() && !localCert.first().isNull()) {
        configuration.setLocalCertificate(localCert.first());
    } else {
        qCritical() << "Could not find certificate: " << certificateFileName;
    }
    return configuration;
}

void tst_QSslServer::testOneSuccessfulConnection()
{
    // Setup server
    QSslConfiguration serverConfiguration = selfSignedServerQSslConfiguration();
    SslServerSpy server(serverConfiguration);
    QVERIFY(server.server.listen());

    // Check that all signal spys are valid
    QVERIFY(server.sslErrorsSpy.isValid());
    QVERIFY(server.peerVerifyErrorSpy.isValid());
    QVERIFY(server.errorOccurredSpy.isValid());
    QVERIFY(server.pendingConnectionAvailableSpy.isValid());
    QVERIFY(server.preSharedKeyAuthenticationRequiredSpy.isValid());
    QVERIFY(server.alertSentSpy.isValid());
    QVERIFY(server.alertReceivedSpy.isValid());
    QVERIFY(server.handshakeInterruptedOnErrorSpy.isValid());
    QVERIFY(server.startedEncryptionHandshakeSpy.isValid());

    // Check that no connections has occurred
    QCOMPARE(server.sslErrorsSpy.count(), 0);
    QCOMPARE(server.peerVerifyErrorSpy.count(), 0);
    QCOMPARE(server.errorOccurredSpy.count(), 0);
    QCOMPARE(server.pendingConnectionAvailableSpy.count(), 0);
    QCOMPARE(server.preSharedKeyAuthenticationRequiredSpy.count(), 0);
    QCOMPARE(server.alertSentSpy.count(), 0);
    QCOMPARE(server.alertReceivedSpy.count(), 0);
    QCOMPARE(server.handshakeInterruptedOnErrorSpy.count(), 0);
    QCOMPARE(server.startedEncryptionHandshakeSpy.count(), 0);

    // Connect client
    QSslSocket client;
    QSslConfiguration clientConfiguration = QSslConfiguration::defaultConfiguration();
    client.setSslConfiguration(clientConfiguration);
    client.connectToHostEncrypted(QHostAddress(QHostAddress::LocalHost).toString(),
                                  server.server.serverPort());

    // Type of certificate error to expect
    const auto certificateError =
            isTestingOpenSsl ? QSslError::SelfSignedCertificate : QSslError::CertificateUntrusted;
    // Expected errors
    connect(&client, &QSslSocket::sslErrors,
            [&certificateError, &client](const QList<QSslError> &errors) {
                QCOMPARE(errors.size(), 2);
                for (auto error : errors) {
                    QVERIFY(error.error() == certificateError
                            || error.error() == QSslError::HostNameMismatch);
                }
                client.ignoreSslErrors();
            });

    QEventLoop loop;
    int waitFor = 2;
    connect(&client, &QSslSocket::encrypted, [&loop, &waitFor]() {
        if (!--waitFor)
            loop.quit();
    });
    connect(&server.server, &QTcpServer::pendingConnectionAvailable, [&loop, &waitFor]() {
        if (!--waitFor)
            loop.quit();
    });
    QTimer::singleShot(5000, &loop, SLOT(quit()));
    loop.exec();

    // Check that one encrypted connection has occurred without error
    QCOMPARE(server.sslErrorsSpy.count(), 0);
    QCOMPARE(server.peerVerifyErrorSpy.count(), 0);
    QCOMPARE(server.errorOccurredSpy.count(), 0);
    QCOMPARE(server.pendingConnectionAvailableSpy.count(), 1);
    QCOMPARE(server.preSharedKeyAuthenticationRequiredSpy.count(), 0);
    QCOMPARE(server.alertSentSpy.count(), 0);
    QCOMPARE(server.alertReceivedSpy.count(), 0);
    QCOMPARE(server.handshakeInterruptedOnErrorSpy.count(), 0);
    QCOMPARE(server.startedEncryptionHandshakeSpy.count(), 1);

    // Check client socket
    QVERIFY(client.isEncrypted());
    QCOMPARE(client.state(), QAbstractSocket::ConnectedState);
}

void tst_QSslServer::testSelfSignedCertificateRejectedByServer()
{
    // Set up server that verifies client
    QSslConfiguration serverConfiguration = selfSignedServerQSslConfiguration();
    serverConfiguration.setPeerVerifyMode(QSslSocket::VerifyPeer);
    SslServerSpy server(serverConfiguration);
    QVERIFY(server.server.listen());

    // Connect client
    QSslSocket client;
    QSslConfiguration clientConfiguration = selfSignedClientQSslConfiguration();
    clientConfiguration.setPeerVerifyMode(QSslSocket::VerifyNone);
    client.setSslConfiguration(clientConfiguration);
    client.connectToHostEncrypted(QHostAddress(QHostAddress::LocalHost).toString(),
                                  server.server.serverPort());

    QEventLoop loop;
    QObject::connect(&client, SIGNAL(disconnected()), &loop, SLOT(quit()));
    QTimer::singleShot(5000, &loop, SLOT(quit()));
    loop.exec();

    // Check that one encrypted connection has failed
    QCOMPARE(server.sslErrorsSpy.count(), 1);
    QCOMPARE(server.peerVerifyErrorSpy.count(), 1);
    QCOMPARE(server.errorOccurredSpy.count(), 1);
    QCOMPARE(server.pendingConnectionAvailableSpy.count(), 0);
    QCOMPARE(server.preSharedKeyAuthenticationRequiredSpy.count(), 0);
    QCOMPARE(server.alertSentSpy.count(),
             isTestingOpenSsl ? 1 : 0); // OpenSSL only signal
    QCOMPARE(server.alertReceivedSpy.count(), 0);
    QCOMPARE(server.handshakeInterruptedOnErrorSpy.count(), 0);
    QCOMPARE(server.startedEncryptionHandshakeSpy.count(), 1);

    // Type of certificate error to expect
    const auto certificateError =
            isTestingOpenSsl ? QSslError::SelfSignedCertificate : QSslError::CertificateUntrusted;

    // Check the sslErrorsSpy
    const auto sslErrorsSpyErrors =
            qvariant_cast<QList<QSslError>>(qAsConst(server.sslErrorsSpy).first()[1]);
    QCOMPARE(sslErrorsSpyErrors.size(), 1);
    QCOMPARE(sslErrorsSpyErrors.first().error(), certificateError);

    // Check the peerVerifyErrorSpy
    const auto peerVerifyErrorSpyError =
            qvariant_cast<QSslError>(qAsConst(server.peerVerifyErrorSpy).first()[1]);
    QCOMPARE(peerVerifyErrorSpyError.error(), certificateError);

    // Check client socket
    QVERIFY(!client.isEncrypted());
    QCOMPARE(client.state(), QAbstractSocket::UnconnectedState);
}

void tst_QSslServer::testSelfSignedCertificateRejectedByClient()
{
    // Set up server without verification of client
    QSslConfiguration serverConfiguration = selfSignedServerQSslConfiguration();
    SslServerSpy server(serverConfiguration);
    QVERIFY(server.server.listen());

    // Connect client that authenticates server
    QSslSocket client;
    QSslConfiguration clientConfiguration = selfSignedClientQSslConfiguration();
    if (isTestingOpenSsl) {
        clientConfiguration.setHandshakeMustInterruptOnError(true);
        QVERIFY(clientConfiguration.handshakeMustInterruptOnError());
    }
    client.setSslConfiguration(clientConfiguration);
    QSignalSpy clientConnectedSpy(&client, SIGNAL(connected()));
    QSignalSpy clientHostFoundSpy(&client, SIGNAL(hostFound()));
    QSignalSpy clientDisconnectedSpy(&client, SIGNAL(disconnected()));
    QSignalSpy clientConnectionEncryptedSpy(&client, SIGNAL(encrypted()));
    QSignalSpy clientSslErrorsSpy(&client, SIGNAL(sslErrors(QList<QSslError>)));
    QSignalSpy clientErrorOccurredSpy(&client, SIGNAL(errorOccurred(QAbstractSocket::SocketError)));
    client.connectToHostEncrypted(QHostAddress(QHostAddress::LocalHost).toString(),
                                  server.server.serverPort());
    QEventLoop loop;
    QTimer::singleShot(1000, &loop, SLOT(quit()));
    loop.exec();

    // Type of socket error to expect
    const auto socketError = isTestingOpenSsl
            ? QAbstractSocket::SocketError::SslHandshakeFailedError
            : QAbstractSocket::SocketError::RemoteHostClosedError;

    QTcpSocket *connection = server.server.nextPendingConnection();
    if (connection == nullptr) {
        // Client disconnected before connection accepted by server
        QCOMPARE(server.sslErrorsSpy.count(), 0);
        QCOMPARE(server.peerVerifyErrorSpy.count(), 0);
        QCOMPARE(server.errorOccurredSpy.count(), 1); // Client rejected first
        QCOMPARE(server.pendingConnectionAvailableSpy.count(), 0);
        QCOMPARE(server.preSharedKeyAuthenticationRequiredSpy.count(), 0);
        QCOMPARE(server.alertSentSpy.count(), 0);
        QCOMPARE(server.alertReceivedSpy.count(),
                 isTestingOpenSsl ? 1 : 0); // OpenSSL only signal
        QCOMPARE(server.handshakeInterruptedOnErrorSpy.count(), 0);
        QCOMPARE(server.startedEncryptionHandshakeSpy.count(), 1);

        const auto errrOccuredSpyError = qvariant_cast<QAbstractSocket::SocketError>(
                qAsConst(server.errorOccurredSpy).first()[1]);
        QCOMPARE(errrOccuredSpyError, socketError);
    } else {
        // Client disconnected after connection accepted by server
        QCOMPARE(server.sslErrorsSpy.count(), 0);
        QCOMPARE(server.peerVerifyErrorSpy.count(), 0);
        QCOMPARE(server.errorOccurredSpy.count(), 0); // Server accepted first
        QCOMPARE(server.pendingConnectionAvailableSpy.count(), 1);
        QCOMPARE(server.preSharedKeyAuthenticationRequiredSpy.count(), 0);
        QCOMPARE(server.alertSentSpy.count(), 0);
        QCOMPARE(server.alertReceivedSpy.count(),
                 isTestingOpenSsl ? 1 : 0); // OpenSSL only signal
        QCOMPARE(server.handshakeInterruptedOnErrorSpy.count(), 0);
        QCOMPARE(server.startedEncryptionHandshakeSpy.count(), 1);

        QCOMPARE(connection->state(), QAbstractSocket::UnconnectedState);
        QCOMPARE(connection->error(), socketError);
        auto sslConnection = qobject_cast<QSslSocket *>(connection);
        QVERIFY(sslConnection);
        QVERIFY(!sslConnection->isEncrypted());
    }

    // Check that client has rejected server
    QCOMPARE(clientConnectedSpy.count(), 1);
    QCOMPARE(clientHostFoundSpy.count(), 1);
    QCOMPARE(clientDisconnectedSpy.count(), 1);
    QCOMPARE(clientConnectionEncryptedSpy.count(), 0);
    QCOMPARE(clientSslErrorsSpy.count(), isTestingOpenSsl ? 0 : 1);
    QCOMPARE(clientErrorOccurredSpy.count(), 1);

    // Check client socket
    QVERIFY(!client.isEncrypted());
    QCOMPARE(client.state(), QAbstractSocket::UnconnectedState);
}

#if QT_CONFIG(openssl)

void tst_QSslServer::testHandshakeInterruptedOnError()
{
    if (!isTestingOpenSsl)
        QSKIP("This test requires OpenSSL as the active TLS backend");

    auto serverConfiguration = selfSignedServerQSslConfiguration();
    serverConfiguration.setHandshakeMustInterruptOnError(true);
    QVERIFY(serverConfiguration.handshakeMustInterruptOnError());
    serverConfiguration.setPeerVerifyMode(QSslSocket::VerifyPeer);
    SslServerSpy server(serverConfiguration);
    server.server.listen();

    QSslSocket client;
    auto clientConfiguration = selfSignedClientQSslConfiguration();
    clientConfiguration.setPeerVerifyMode(QSslSocket::VerifyNone);
    client.setSslConfiguration(clientConfiguration);
    client.connectToHostEncrypted(QHostAddress(QHostAddress::LocalHost).toString(),
                                  server.server.serverPort());

    QEventLoop loop;
    QObject::connect(&client, SIGNAL(disconnected()), &loop, SLOT(quit()));
    QTimer::singleShot(5000, &loop, SLOT(quit()));
    loop.exec();

    // Check that client certificate causes handshake interrupted signal to be emitted
    QCOMPARE(server.sslErrorsSpy.count(), 0);
    QCOMPARE(server.peerVerifyErrorSpy.count(), 0);
    QCOMPARE(server.errorOccurredSpy.count(), 1);
    QCOMPARE(server.pendingConnectionAvailableSpy.count(), 0);
    QCOMPARE(server.preSharedKeyAuthenticationRequiredSpy.count(), 0);
    QCOMPARE(server.alertSentSpy.count(), 1);
    QCOMPARE(server.alertReceivedSpy.count(), 0);
    QCOMPARE(server.handshakeInterruptedOnErrorSpy.count(), 1);
    QCOMPARE(server.startedEncryptionHandshakeSpy.count(), 1);
}

void tst_QSslServer::testPreSharedKeyAuthenticationRequired()
{
    if (!isTestingOpenSsl)
        QSKIP("This test requires OpenSSL as the active TLS backend");

    auto serverConfiguration = QSslConfiguration::defaultConfiguration();
    serverConfiguration.setPeerVerifyMode(QSslSocket::VerifyPeer);
    serverConfiguration.setProtocol(QSsl::TlsV1_2);
    serverConfiguration.setCiphers({ QSslCipher("PSK-AES256-CBC-SHA") });
    serverConfiguration.setPreSharedKeyIdentityHint("Server Y");
    SslServerSpy server(serverConfiguration);
    connect(&server.server, &QSslServer::preSharedKeyAuthenticationRequired,
            [](QSslSocket *, QSslPreSharedKeyAuthenticator *authenticator) {
                QCOMPARE(authenticator->identity(), QByteArray("Client X"));
                authenticator->setPreSharedKey("123456");
            });
    server.server.listen();

    QSslSocket client;
    auto clientConfiguration = QSslConfiguration::defaultConfiguration();
    clientConfiguration.setPeerVerifyMode(QSslSocket::VerifyNone);
    clientConfiguration.setProtocol(QSsl::TlsV1_2);
    clientConfiguration.setCiphers({ QSslCipher("PSK-AES256-CBC-SHA") });
    client.setSslConfiguration(clientConfiguration);
    connect(&client, &QSslSocket::preSharedKeyAuthenticationRequired,
            [](QSslPreSharedKeyAuthenticator *authenticator) {
                QCOMPARE(authenticator->identityHint(), QByteArray("Server Y"));
                authenticator->setPreSharedKey("123456");
                authenticator->setIdentity("Client X");
            });
    client.connectToHostEncrypted(QHostAddress(QHostAddress::LocalHost).toString(),
                                  server.server.serverPort());

    connect(&server.server, &QSslServer::sslErrors,
            [](QSslSocket *socket, const QList<QSslError> &errors) {
                for (auto error : errors) {
                    QCOMPARE(error.error(), QSslError::NoPeerCertificate);
                }
                socket->ignoreSslErrors();
            });

    QEventLoop loop;
    QObject::connect(&client, SIGNAL(encrypted()), &loop, SLOT(quit()));
    QTimer::singleShot(5000, &loop, SLOT(quit()));
    loop.exec();

    // Check that server is connected
    QCOMPARE(server.sslErrorsSpy.count(), 1);
    QCOMPARE(server.peerVerifyErrorSpy.count(), 1);
    QCOMPARE(server.errorOccurredSpy.count(), 0);
    QCOMPARE(server.pendingConnectionAvailableSpy.count(), 1);
    QCOMPARE(server.preSharedKeyAuthenticationRequiredSpy.count(), 1);
    QCOMPARE(server.alertSentSpy.count(), 0);
    QCOMPARE(server.alertReceivedSpy.count(), 0);
    QCOMPARE(server.handshakeInterruptedOnErrorSpy.count(), 0);
    QCOMPARE(server.startedEncryptionHandshakeSpy.count(), 1);

    // Check client socket
    QVERIFY(client.isEncrypted());
    QCOMPARE(client.state(), QAbstractSocket::ConnectedState);
}

#endif

QTEST_MAIN(tst_QSslServer)

#include "tst_qsslserver.moc"
