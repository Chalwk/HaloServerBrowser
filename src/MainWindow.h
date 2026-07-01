// Copyright (c) 2026 Jericho Crosby (Chalwk).
// Licensed under the GPL License.

#pragma once

#include <QMainWindow>
#include <QTableWidget>
#include <QLineEdit>
#include <QComboBox>
#include <QLabel>
#include <QPushButton>
#include <QProcess>
#include <QList>
#include <QMap>
#include <QDateTime>
#include "ServerItem.h"

QT_BEGIN_NAMESPACE
namespace Ui
{
    class MainWindow;
}
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private slots:
    void onRefreshClicked();
    void onFilterChanged();
    void onSortChanged();
    void onPresetChanged(int index);
    void onTableClicked(int row, int column);
    void onPlayerButtonClicked();
    void onRulesButtonClicked();
    void onGslistFinished(int exitCode, QProcess::ExitStatus status);
    void onDetailsFinished(int exitCode, QProcess::ExitStatus status);

private:
    Ui::MainWindow *ui;
    QList<ServerItem> m_allServers;      // full list from master
    QList<ServerItem> m_filteredServers; // after filtering/sorting
    QTableWidget *m_table;
    QLineEdit *m_masterEdit;
    QLineEdit *m_gameNameEdit;
    QLineEdit *m_gameKeyEdit;
    QLineEdit *m_queryTypeEdit;
    QComboBox *m_presetCombo;
    QPushButton *m_refreshBtn;
    QLabel *m_statusLabel;
    QLineEdit *m_nameFilter;
    QComboBox *m_gameTypeFilter;
    QComboBox *m_pwdFilter;
    QComboBox *m_sortCombo;

    QProcess *m_gslistProcess;
    QProcess *m_detailsProcess;
    QString m_currentDetailsIpPort; // for player/rules dialogs

    // cache for server details
    struct CachedDetails
    {
        QStringList players;
        QMap<QString, QString> rules;
        QDateTime timestamp;
    };
    QMap<QString, CachedDetails> m_detailsCache;
    static constexpr int CACHE_TTL_SECONDS = 60;

    void setupUI();
    void populateTable(const QList<ServerItem> &servers);
    void applyFiltersAndSort();
    void runGslistMaster();
    void parseMasterOutput(const QString &output);
    void requestServerDetails(const QString &ipPort, bool forPlayers);
    void parseServerDetails(const QString &output, QStringList &players, QMap<QString, QString> &rules);
    void showDetailsDialog(const QString &ipPort, bool showPlayers);
    QProcess *createGslistProcess();

    bool getCachedDetails(const QString &ipPort, QStringList &players, QMap<QString, QString> &rules);
    void setCachedDetails(const QString &ipPort, const QStringList &players, const QMap<QString, QString> &rules);
};