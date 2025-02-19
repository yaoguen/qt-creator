// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0+ OR GPL-3.0 WITH Qt-GPL-exception-1.0

#pragma once

#include <coreplugin/dialogs/ioptionspage.h>

#include <utils/aspects.h>

QT_BEGIN_NAMESPACE
class QDir;
QT_END_NAMESPACE

namespace Perforce {
namespace Internal {

/* PerforceSettings: Aggregates settings items and toplevel directory
 * which is determined externally by background checks and provides a convenience
 * for determining the common arguments.
 * Those must contain (apart from server connection settings) the working directory
 * with the "-d" option. This is because the p4 command line client detects its path
 * from the PWD environment variable which breaks relative paths since that is set by
 * the shell running Creator and is not necessarily that of the working directory
 * (see p4 documentation).
 * An additional complication is that the repository might be a symbolic link on Unix,
 * say "$HOME/dev" linked to "/depot". If the p4 client specification contains
 * "$HOME/dev", paths containing "/depot" will be refused as "not under client's view" by
 * p4. This is why the client root portion of working directory must be mapped for the
 * "-d" option, so that running p4 in "/depot/dev/foo" results in "-d $HOME/dev/foo". */

class PerforceSettings : public Utils::AspectContainer
{
    Q_DECLARE_TR_FUNCTIONS(Perforce::Internal::SettingsPage)

public:
    PerforceSettings();
    ~PerforceSettings();

    bool isValid() const;

    // Checks. On success, errorMessage will contains the client root.
    bool check(QString *repositoryRoot /* = 0*/, QString *errorMessage) const;
    static bool doCheck(const QString &binary, const QStringList &basicArgs,
                        QString *repositoryRoot /* = 0 */,
                        QString *errorMessage);

    int longTimeOutS() const { return timeOutS.value() * 10; }
    int timeOutMS() const { return timeOutS.value() * 1000;  }

    Utils::FilePath topLevel() const;
    Utils::FilePath topLevelSymLinkTarget() const;

    void setTopLevel(const QString &);

    // Return relative path to top level. Returns "" if it is the same directory,
    // ".." if it is not within.
    QString relativeToTopLevel(const QString &dir) const;
    // Return argument list relative to top level (empty meaning,
    // it is the same directory).
    QString relativeToTopLevelArguments(const QString &dir) const;

    // Map p4 path back to file system in case of a symlinked top-level
    QString mapToFileSystem(const QString &perforceFilePath) const;

    bool defaultEnv() const;

    // Return basic arguments, including -d and server connection parameters.
    QStringList commonP4Arguments() const;
    QStringList commonP4Arguments(const QString &workingDir) const;

    void clearTopLevel();

    Utils::StringAspect p4BinaryPath;
    Utils::StringAspect p4Port;
    Utils::StringAspect p4Client;
    Utils::StringAspect p4User;
    Utils::IntegerAspect logCount;
    Utils::BoolAspect customEnv;
    Utils::IntegerAspect timeOutS;
    Utils::BoolAspect autoOpen;

private:
    QStringList workingDirectoryArguments(const QString &workingDir) const;
    QString m_topLevel;
    QString m_topLevelSymLinkTarget;
    QDir *m_topLevelDir = nullptr;
};

class PerforceSettingsPage final : public Core::IOptionsPage
{
public:
    explicit PerforceSettingsPage(PerforceSettings *settings);
};

} // namespace Internal
} // namespace Perforce
