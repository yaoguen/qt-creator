// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0+ OR GPL-3.0 WITH Qt-GPL-exception-1.0

#include "dumpsender.h"

#include <QCoreApplication>
#include <QDateTime>
#include <QDebug>
#include <QFile>
#include <QFileInfo>
#include <QHttpMultiPart>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QPair>
#include <QProcess>
#include <QStringList>
#include <QTemporaryFile>
#include <QUrl>

DumpSender::DumpSender(QObject *parent) :
    QObject(parent),
    m_httpMultiPart(QHttpMultiPart::FormDataType)
{
    const QString dumpPath = QCoreApplication::arguments().at(1);
    const QByteArray startupTime = QCoreApplication::arguments().at(2).toLocal8Bit();
    const QByteArray applicationName = QCoreApplication::arguments().at(3).toLocal8Bit();
    QByteArray applicationVersion = QCoreApplication::arguments().at(4).toLocal8Bit();
    const QByteArray plugins = QCoreApplication::arguments().at(5).toLocal8Bit();
    // QByteArray ideRevision = QCoreApplication::arguments().at(6).toLocal8Bit();
    m_applicationFilePath = QCoreApplication::arguments().at(7);

    if (applicationVersion.isEmpty())
        applicationVersion = "1.0.0";

    QFile dumpFile(dumpPath, this);
    const bool isOpen = dumpFile.open(QIODevice::ReadOnly);
    Q_ASSERT(isOpen);
    Q_UNUSED(isOpen);

    const QList<QPair<QByteArray, QByteArray> > pairList = {
        { "StartupTime", startupTime },
        { "Vendor", "Qt Project" },
        { "InstallTime", "0" },
        { "Add-ons", plugins },
        { "BuildID", "" },
        { "SecondsSinceLastCrash", "0" },
        { "ProductName", applicationName },
        { "URL", "" },
        { "Theme", "" },
        { "Version", applicationVersion },
        { "CrashTime", QByteArray::number(QDateTime::currentDateTime().toTime_t()) },
        { "Throttleable", "0" }
    };

    const QByteArray boundary = m_httpMultiPart.boundary();
    m_formData.append("--" + boundary + "\r\n");
    for (const auto &pair : pairList) {
        m_formData.append("Content-Disposition: form-data; name=\"" + pair.first + "\"\r\n\r\n");
        m_formData.append(pair.second + "\r\n");
        m_formData.append("--" + boundary + "\r\n");
    }


    QByteArray dumpArray = dumpFile.readAll();
    m_formData.append("Content-Type: application/octet-stream\r\n");
    m_formData.append("Content-Disposition: form-data; name=\"upload_file_minidump\"; filename=\""
                      + QFileInfo(dumpPath).baseName().toUtf8() + "\r\n");
    m_formData.append("Content-Transfer-Encoding: binary\r\n\r\n");
    m_formData.append(dumpArray);

    m_formData.append("--" + boundary + "--\r\n");

    for (const auto &pair : pairList) {
        QHttpPart httpPart;
        httpPart.setHeader(QNetworkRequest::ContentDispositionHeader, "form-data; name=\"" + pair.first + "\"");
        httpPart.setBody(pair.second);
        m_httpMultiPart.append(httpPart);
    }

    QHttpPart dumpPart;
    dumpPart.setHeader(QNetworkRequest::ContentTypeHeader, "application/octet-stream");
    dumpPart.setHeader(QNetworkRequest::ContentDispositionHeader,
                       "form-data; name=\"upload_file_minidump\"; filename=\""
                       + QFileInfo(dumpPath).baseName().toUtf8() + "\"");
    dumpPart.setRawHeader("Content-Transfer-Encoding:", "binary");
    dumpPart.setBody(dumpArray);
    m_httpMultiPart.append(dumpPart);
}

void DumpSender::sendDumpAndQuit()
{
    QNetworkAccessManager *manager = new QNetworkAccessManager(this);

    QNetworkRequest request(QUrl("http://crashes.qt.io/submit"));

    const QByteArray boundary = m_httpMultiPart.boundary();
    request.setHeader(QNetworkRequest::ContentTypeHeader, "multipart/form-data; boundary=" + boundary);

    QList<QPair<QByteArray, QByteArray>> pairList;

    if (!m_emailAddress.isEmpty())
        pairList.append({ "Email", m_emailAddress.toLocal8Bit() });

    if (!m_commentText.isEmpty())
        pairList.append({ "Comments", m_commentText.toLocal8Bit() });

    for (const auto &pair : pairList) {
        m_formData.append("Content-Disposition: form-data; name=\"" + pair.first + "\"\r\n\r\n");
        m_formData.append(pair.second + "\r\n");
        m_formData.append("--" + boundary + "\r\n");
    }

    for (const auto &pair : pairList) {
        QHttpPart httpPart;
        httpPart.setHeader(QNetworkRequest::ContentDispositionHeader, "form-data; name=\"" + pair.first + "\"");
        httpPart.setBody(pair.second);
        m_httpMultiPart.append(httpPart);
    }

    QNetworkReply *reply = manager->post(request, &m_httpMultiPart);

    m_httpMultiPart.setParent(reply);

    connect(reply, &QNetworkReply::uploadProgress, this, &DumpSender::uploadProgress);
    connect(reply, &QNetworkReply::finished, QCoreApplication::instance(), &QCoreApplication::quit);
    connect(reply, QOverload<QNetworkReply::NetworkError>::of(&QNetworkReply::error),
            QCoreApplication::instance(), &QCoreApplication::quit);
}

void DumpSender::restartCrashedApplicationAndSendDump()
{
    QProcess::startDetached(m_applicationFilePath);
    sendDumpAndQuit();
}

void DumpSender::restartCrashedApplication()
{
    QProcess::startDetached(m_applicationFilePath);
    QCoreApplication::quit();
}

void DumpSender::setEmailAddress(const QString &email)
{
    m_emailAddress = email;
}

void DumpSender::setCommentText(const QString &comment)
{
    m_commentText = comment;
}

int DumpSender::dumperSize() const
{
    return m_formData.size();
}
