// Copyright (c) 2026 Jericho Crosby (Chalwk).
// Licensed under the GPL License.

#pragma once

#include <QDialog>
#include <QStringList>
#include <QMap>
#include <QString>

class ServerDetailsDialog : public QDialog
{
    Q_OBJECT

public:
    ServerDetailsDialog(const QString &ipPort,
                        const QStringList &players,
                        const QMap<QString, QString> &rules,
                        bool showPlayers,
                        QWidget *parent = nullptr);
    ~ServerDetailsDialog();

private:
    void setupUI(const QString &ipPort,
                 const QStringList &players,
                 const QMap<QString, QString> &rules,
                 bool showPlayers);
};