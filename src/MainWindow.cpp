// Copyright (c) 2026 Jericho Crosby (Chalwk).
// Licensed under the GPL License.

#include "MainWindow.h"
#include "ServerDetailsDialog.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGroupBox>
#include <QHeaderView>
#include <QMessageBox>
#include <QPushButton>
#include <QLabel>
#include <QComboBox>
#include <QLineEdit>
#include <QProcess>
#include <QFile>
#include <QDir>
#include <QRegularExpression>
#include <QDateTime>
#include <QCoreApplication>
#include <QTimer>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent), ui(nullptr), m_gslistProcess(nullptr), m_detailsProcess(nullptr)
{
    setupUI();
    m_presetCombo->setCurrentIndex(4);
    onPresetChanged(4);
    QTimer::singleShot(500, this, &MainWindow::onRefreshClicked);
}

MainWindow::~MainWindow()
{
    if (m_gslistProcess && m_gslistProcess->state() != QProcess::NotRunning)
        m_gslistProcess->terminate();
    if (m_detailsProcess && m_detailsProcess->state() != QProcess::NotRunning)
        m_detailsProcess->terminate();
}

void MainWindow::setupUI()
{
    setWindowTitle("Halo Server Browser");
    resize(1200, 700);

    QWidget *central = new QWidget(this);
    setCentralWidget(central);
    QVBoxLayout *mainLayout = new QVBoxLayout(central);

    // ---- Params row ----
    QGroupBox *paramsGroup = new QGroupBox("Parameters");
    QVBoxLayout *paramsLayout = new QVBoxLayout(paramsGroup);

    auto addParamRow = [&](const QString &labelText, QWidget *widget)
    {
        QHBoxLayout *row = new QHBoxLayout;
        QLabel *label = new QLabel(labelText);
        label->setFixedWidth(120);
        row->addWidget(label);
        row->addWidget(widget);
        paramsLayout->addLayout(row);
    };

    m_masterEdit = new QLineEdit("34.197.71.170:28910");
    addParamRow("Master server (-x):", m_masterEdit);

    m_presetCombo = new QComboBox;

    // Game presets
    QStringList presets;
    presets << "Halo Beta|halo|QW88cv"
            << "Halo Demo|halod|yG3d9w"
            << "Halo Demo (Mac)|halomacd|e4Rd9J"
            << "Halo Mac|halomac|e4Rd9J"
            << "Halo Custom Edition|halom|e4Rd9J"
            << "Halo: Combat Evolved (PC)|halor|e4Rd9J";
    for (const QString &p : presets) {
        QStringList parts = p.split('|');
        QString display = parts[0];
        QString key = parts[1] + "|" + parts[2];
        m_presetCombo->addItem(display, key);
    }
    connect(m_presetCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &MainWindow::onPresetChanged);
    addParamRow("Game preset:", m_presetCombo);

    m_gameNameEdit = new QLineEdit("halom");
    addParamRow("Game name (-n):", m_gameNameEdit);

    m_gameKeyEdit = new QLineEdit("halom e4Rd9J");
    addParamRow("Game key (-Y):", m_gameKeyEdit);

    m_queryTypeEdit = new QLineEdit("8");
    QHBoxLayout *qtypeRow = new QHBoxLayout;
    qtypeRow->addWidget(new QLabel("Query type (-Q):"));
    qtypeRow->addWidget(m_queryTypeEdit);
    qtypeRow->addWidget(new QLabel("(8 = full server details)"));
    qtypeRow->addStretch();
    paramsLayout->addLayout(qtypeRow);

    mainLayout->addWidget(paramsGroup);

    // Controls
    QHBoxLayout *controlsLayout = new QHBoxLayout;
    m_refreshBtn = new QPushButton("🔄 Refresh Servers");
    m_refreshBtn->setMinimumWidth(150);
    connect(m_refreshBtn, &QPushButton::clicked, this, &MainWindow::onRefreshClicked);
    controlsLayout->addWidget(m_refreshBtn);

    m_statusLabel = new QLabel("Ready. Click Refresh to query master server.");
    controlsLayout->addWidget(m_statusLabel);
    controlsLayout->addStretch();
    mainLayout->addLayout(controlsLayout);

    // Filter bar
    QHBoxLayout *filterLayout = new QHBoxLayout;
    filterLayout->addWidget(new QLabel("🔍 Filter by name/map:"));
    m_nameFilter = new QLineEdit;
    m_nameFilter->setPlaceholderText("Server name or map...");
    connect(m_nameFilter, &QLineEdit::textChanged, this, &MainWindow::onFilterChanged);
    filterLayout->addWidget(m_nameFilter);

    filterLayout->addWidget(new QLabel("🎮 Game Type:"));
    m_gameTypeFilter = new QComboBox;
    m_gameTypeFilter->addItems({"All", "CTF", "Slayer", "Race", "King", "Oddball"});
    connect(m_gameTypeFilter, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &MainWindow::onFilterChanged);
    filterLayout->addWidget(m_gameTypeFilter);

    filterLayout->addWidget(new QLabel("🔒 Password:"));
    m_pwdFilter = new QComboBox;
    m_pwdFilter->addItems({"Any", "No Password", "Password Protected"});
    connect(m_pwdFilter, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &MainWindow::onFilterChanged);
    filterLayout->addWidget(m_pwdFilter);

    filterLayout->addWidget(new QLabel("📊 Sort by:"));
    m_sortCombo = new QComboBox;
    m_sortCombo->addItems({"Ping (lowest first)", "Players (full first)", "Server Name", "Map Name"});
    connect(m_sortCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &MainWindow::onSortChanged);
    filterLayout->addWidget(m_sortCombo);

    filterLayout->addStretch();
    mainLayout->addLayout(filterLayout);

    // Table (list)
    m_table = new QTableWidget;
    m_table->setColumnCount(9);
    QStringList headers = {"🏠 Server Name", "🗺️ Map", "🎯 Game Type",
                           "👥 Players", "📶 Ping", "🔐 Pass", "🌐 IP:Port",
                           "👥 Players", "⚙️ Rules"};
    m_table->setHorizontalHeaderLabels(headers);
    m_table->horizontalHeader()->setSectionResizeMode(QHeaderView::ResizeToContents);
    m_table->horizontalHeader()->setStretchLastSection(false);
    m_table->setEditTriggers(QTableWidget::NoEditTriggers);
    m_table->setSelectionBehavior(QTableWidget::SelectRows);
    m_table->setSortingEnabled(true);
    m_table->horizontalHeader()->setSortIndicatorShown(true);
    connect(m_table, &QTableWidget::cellClicked, this, &MainWindow::onTableClicked);
    mainLayout->addWidget(m_table);

    // Footer
    QLabel *footer = new QLabel("Copyright (c) 2026. Jericho Crosby (Chalwk) · Halo Server Browser (Qt port)");
    footer->setAlignment(Qt::AlignCenter);
    mainLayout->addWidget(footer);
}

void MainWindow::onPresetChanged(int index)
{
    if (index < 0 || index >= m_presetCombo->count())
        return;
    QString presetKey = m_presetCombo->itemData(index).toString();
    if (presetKey.isEmpty())
        return;
    QStringList parts = presetKey.split('|');
    if (parts.size() == 2)
    {
        QString game = parts[0];
        QString key = parts[1];
        m_gameNameEdit->setText(game);
        m_gameKeyEdit->setText(game + " " + key);
    }
}

void MainWindow::onRefreshClicked()
{
    m_refreshBtn->setEnabled(false);
    m_refreshBtn->setText("⏳ Running...");
    m_statusLabel->setText("Querying master server...");
    runGslistMaster();
}

void MainWindow::runGslistMaster()
{
    // Check if gslist.exe exists in the app directory
    QString appDir = QCoreApplication::applicationDirPath();
    QString gslistPath = appDir + "/gslist.exe";
    if (!QFile::exists(gslistPath))
    {
        m_statusLabel->setText("❌ gslist.exe not found in application directory.");
        m_refreshBtn->setEnabled(true);
        m_refreshBtn->setText("🔄 Refresh Servers");
        QMessageBox::critical(this, "Error",
            "gslist.exe not found in the application directory.\n"
            "Please place gslist.exe in the same folder as the executable.");
        return;
    }

    if (m_gslistProcess && m_gslistProcess->state() != QProcess::NotRunning)
    {
        m_gslistProcess->terminate();
        m_gslistProcess->waitForFinished(1000);
    }
    delete m_gslistProcess;
    m_gslistProcess = createGslistProcess();

    QString master = m_masterEdit->text().trimmed();
    QString game = m_gameNameEdit->text().trimmed();
    QString key = m_gameKeyEdit->text().trimmed();
    QString qtype = m_queryTypeEdit->text().trimmed();

    QStringList args;
    args << "-x" << master
         << "-n" << game
         << "-Y" << key.split(' ').first() << key.section(' ', 1)
         << "-Q" << qtype
         << "-q";

    // Connect finished signal
    connect(m_gslistProcess, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
            this, &MainWindow::onGslistFinished);

    m_gslistProcess->start("gslist.exe", args);
    if (!m_gslistProcess->waitForStarted(3000))
    {
        m_statusLabel->setText("❌ Failed to start gslist.exe.");
        m_refreshBtn->setEnabled(true);
        m_refreshBtn->setText("🔄 Refresh Servers");
        QMessageBox::critical(this, "Error", "Could not start gslist.exe.");
        return;
    }

    // Set a timeout: if process doesn't finish within 30 seconds, kill it
    QTimer::singleShot(30000, this, [this]() {
        if (m_gslistProcess && m_gslistProcess->state() == QProcess::Running) {
            m_gslistProcess->terminate();
            m_gslistProcess->waitForFinished(1000);
            m_statusLabel->setText("❌ GSList timeout (30s).");
            m_refreshBtn->setEnabled(true);
            m_refreshBtn->setText("🔄 Refresh Servers");
        }
    });
}

void MainWindow::onGslistFinished(int exitCode, QProcess::ExitStatus status)
{
    m_refreshBtn->setEnabled(true);
    m_refreshBtn->setText("🔄 Refresh Servers");

    if (status != QProcess::NormalExit || exitCode != 0)
    {
        QString err = m_gslistProcess->readAllStandardError();
        m_statusLabel->setText("❌ GSList error: " + (err.isEmpty() ? "Unknown error" : err));
        m_allServers.clear();
        applyFiltersAndSort();
        return;
    }

    QString output = m_gslistProcess->readAllStandardOutput();
    parseMasterOutput(output);
    m_statusLabel->setText(QString("✅ Found %1 servers.").arg(m_allServers.size()));
    applyFiltersAndSort();
}

void MainWindow::parseMasterOutput(const QString &output)
{
    m_allServers.clear();
    QStringList lines = output.split('\n', Qt::SkipEmptyParts);
    for (const QString &line : lines)
    {
        QString trimmed = line.trimmed();
        if (trimmed.isEmpty())
            continue;
        int firstSpace = trimmed.indexOf(' ');
        if (firstSpace == -1)
            continue;
        QString ipPort = trimmed.left(firstSpace).trimmed();
        QString rest = trimmed.mid(firstSpace + 1).trimmed();
        if (!rest.startsWith('\\'))
            continue;
        rest = rest.mid(1);

        QStringList parts = rest.split('\\');
        QMap<QString, QString> data;
        for (int i = 0; i < parts.size() - 1; i += 2)
        {
            data[parts[i].toLower()] = parts[i + 1];
        }

        ServerItem s;
        s.ipPort = ipPort;
        QStringList ipPortParts = ipPort.split(':');
        s.ip = ipPortParts.value(0);
        s.port = ipPortParts.value(1, "2302");
        s.hostname = data.value("hostname", "Unknown");
        s.mapname = data.value("mapname", "unknown");
        s.gametype = data.value("gametype", "?");
        s.gamevariant = data.value("gamevariant", "");
        s.numplayers = data.value("numplayers").toInt();
        s.maxplayers = data.value("maxplayers").toInt();
        s.password = (data.value("password") == "1");
        s.ping = data.value("ping").toInt();

        m_allServers.append(s);
    }
}

void MainWindow::applyFiltersAndSort()
{
    m_filteredServers.clear();

    QString nameFilter = m_nameFilter->text().trimmed().toLower();
    QString gameTypeFilter = m_gameTypeFilter->currentText();
    if (gameTypeFilter == "All")
        gameTypeFilter.clear();
    int pwdIndex = m_pwdFilter->currentIndex();
    QString sortKey = m_sortCombo->currentText();

    for (const ServerItem &s : m_allServers)
    {
        bool nameMatch = s.hostname.toLower().contains(nameFilter) ||
                         s.mapname.toLower().contains(nameFilter);
        if (!nameMatch)
            continue;

        if (!gameTypeFilter.isEmpty() && s.gametype != gameTypeFilter)
            continue;

        if (pwdIndex == 1 && s.password)
            continue;
        if (pwdIndex == 2 && !s.password)
            continue;

        m_filteredServers.append(s);
    }

    std::sort(m_filteredServers.begin(), m_filteredServers.end(),
              [&](const ServerItem &a, const ServerItem &b)
              {
                  if (sortKey.startsWith("Ping"))
                      return a.ping < b.ping;
                  if (sortKey.startsWith("Players"))
                  {
                      double ratioA = (a.maxplayers > 0) ? (double)a.numplayers / a.maxplayers : 0;
                      double ratioB = (b.maxplayers > 0) ? (double)b.numplayers / b.maxplayers : 0;
                      return ratioA > ratioB;
                  }
                  if (sortKey.startsWith("Server"))
                      return a.hostname.compare(b.hostname, Qt::CaseInsensitive) < 0;
                  if (sortKey.startsWith("Map"))
                      return a.mapname.compare(b.mapname, Qt::CaseInsensitive) < 0;
                  return false;
              });

    populateTable(m_filteredServers);
}

void MainWindow::populateTable(const QList<ServerItem> &servers)
{
    m_table->setSortingEnabled(false);
    m_table->setRowCount(servers.size());

    for (int row = 0; row < servers.size(); ++row)
    {
        const ServerItem &s = servers.at(row);

        QTableWidgetItem *nameItem = new QTableWidgetItem(s.hostname);
        nameItem->setData(Qt::UserRole, QVariant::fromValue(s));
        m_table->setItem(row, 0, nameItem);
        m_table->setItem(row, 1, new QTableWidgetItem(s.mapname));
        m_table->setItem(row, 2, new QTableWidgetItem(s.gametype));

        QString playersText = QString("%1 / %2").arg(s.numplayers).arg(s.maxplayers);
        m_table->setItem(row, 3, new QTableWidgetItem(playersText));

        QTableWidgetItem *pingItem = new QTableWidgetItem(QString::number(s.ping));
        if (s.ping <= 150)
            pingItem->setForeground(Qt::green);
        else if (s.ping <= 300)
            pingItem->setForeground(Qt::yellow);
        else
            pingItem->setForeground(Qt::red);
        m_table->setItem(row, 4, pingItem);

        m_table->setItem(row, 5, new QTableWidgetItem(s.password ? "🔒 Yes" : "🔓 No"));
        m_table->setItem(row, 6, new QTableWidgetItem(s.ipPort));

        QPushButton *playerBtn = new QPushButton("👥 Players");
        playerBtn->setProperty("row", row);
        connect(playerBtn, &QPushButton::clicked, this, &MainWindow::onPlayerButtonClicked);
        m_table->setCellWidget(row, 7, playerBtn);

        QPushButton *rulesBtn = new QPushButton("⚙️ Rules");
        rulesBtn->setProperty("row", row);
        connect(rulesBtn, &QPushButton::clicked, this, &MainWindow::onRulesButtonClicked);
        m_table->setCellWidget(row, 8, rulesBtn);
    }

    m_table->setSortingEnabled(true);
    m_table->sortItems(4, Qt::AscendingOrder);
}

void MainWindow::onFilterChanged()
{
    applyFiltersAndSort();
}

void MainWindow::onSortChanged()
{
    applyFiltersAndSort();
}

void MainWindow::onTableClicked(int row, int column)
{
    Q_UNUSED(row);
    Q_UNUSED(column);
}

void MainWindow::onPlayerButtonClicked()
{
    QPushButton *btn = qobject_cast<QPushButton *>(sender());
    if (!btn)
        return;
    int row = btn->property("row").toInt();
    if (row < 0 || row >= m_filteredServers.size())
        return;
    const ServerItem &s = m_filteredServers.at(row);
    showDetailsDialog(s.ipPort, true);
}

void MainWindow::onRulesButtonClicked()
{
    QPushButton *btn = qobject_cast<QPushButton *>(sender());
    if (!btn)
        return;
    int row = btn->property("row").toInt();
    if (row < 0 || row >= m_filteredServers.size())
        return;
    const ServerItem &s = m_filteredServers.at(row);
    showDetailsDialog(s.ipPort, false);
}

void MainWindow::showDetailsDialog(const QString &ipPort, bool showPlayers)
{
    QStringList players;
    QMap<QString, QString> rules;

    if (getCachedDetails(ipPort, players, rules))
    {
        ServerDetailsDialog dialog(ipPort, players, rules, showPlayers, this);
        dialog.exec();
        return;
    }

    m_currentDetailsIpPort = ipPort;
    if (m_detailsProcess && m_detailsProcess->state() != QProcess::NotRunning)
    {
        m_detailsProcess->terminate();
        m_detailsProcess->waitForFinished(1000);
    }
    delete m_detailsProcess;
    m_detailsProcess = createGslistProcess();

    QString host = ipPort.split(':').value(0);
    QString port = ipPort.split(':').value(1, "2302");
    QStringList args;
    args << "-i" << host << port << "-q";

    m_detailsProcess->setProperty("showPlayers", showPlayers);
    m_detailsProcess->setProperty("ipPort", ipPort);

    connect(m_detailsProcess, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
            this, &MainWindow::onDetailsFinished);

    m_detailsProcess->start("gslist.exe", args);
    if (!m_detailsProcess->waitForStarted(3000))
    {
        QMessageBox::critical(this, "Error", "Could not start gslist.exe for details.");
        return;
    }
}

void MainWindow::onDetailsFinished(int exitCode, QProcess::ExitStatus status)
{
    if (status != QProcess::NormalExit || exitCode != 0)
    {
        QMessageBox::warning(this, "Error", "Failed to retrieve server details.");
        return;
    }

    QString output = m_detailsProcess->readAllStandardOutput();
    QStringList players;
    QMap<QString, QString> rules;
    parseServerDetails(output, players, rules);

    QString ipPort = m_detailsProcess->property("ipPort").toString();
    bool showPlayers = m_detailsProcess->property("showPlayers").toBool();

    setCachedDetails(ipPort, players, rules);

    ServerDetailsDialog dialog(ipPort, players, rules, showPlayers, this);
    dialog.exec();
}

void MainWindow::parseServerDetails(const QString &output, QStringList &players, QMap<QString, QString> &rules)
{
    players.clear();
    rules.clear();

    if (output.isEmpty())
        return;

    if (output.startsWith('\\'))
    {
        QStringList parts = output.split('\\');
        QMap<QString, QString> data;
        for (int i = 1; i < parts.size() - 1; i += 2)
        {
            data[parts[i].toLower()] = parts[i + 1];
        }

        QList<int> playerIndices;
        for (auto it = data.begin(); it != data.end(); ++it)
        {
            QString key = it.key();
            if (key.startsWith("player"))
            {
                int idx = -1;
                QRegularExpression rx("player[_-]?(\\d+)");
                QRegularExpressionMatch match = rx.match(key);
                if (match.hasMatch())
                {
                    idx = match.captured(1).toInt();
                }
                else if (key == "player")
                {
                    idx = 0;
                }
                if (idx >= 0)
                {
                    playerIndices.append(idx);
                }
            }
        }
        std::sort(playerIndices.begin(), playerIndices.end());
        for (int idx : playerIndices)
        {
            QString key = (idx == 0) ? "player" : QString("player%1").arg(idx);
            if (data.contains(key) && !data[key].isEmpty())
                players.append(data[key]);
        }

        for (auto it = data.begin(); it != data.end(); ++it)
        {
            QString key = it.key();
            if (!key.startsWith("player"))
            {
                rules[key] = it.value();
            }
        }
        return;
    }

    QStringList lines = output.split('\n', Qt::SkipEmptyParts);
    for (const QString &line : lines)
    {
        QString trimmed = line.trimmed();
        if (trimmed.isEmpty())
            continue;
        int space = trimmed.indexOf(' ');
        if (space == -1)
            continue;
        QString key = trimmed.left(space).toLower();
        QString value = trimmed.mid(space + 1).trimmed();
        if (key.startsWith("player_"))
        {
            players.append(value);
        }
        else
        {
            rules[key] = value;
        }
    }
}

bool MainWindow::getCachedDetails(const QString &ipPort, QStringList &players, QMap<QString, QString> &rules)
{
    if (!m_detailsCache.contains(ipPort))
        return false;
    CachedDetails cache = m_detailsCache[ipPort];
    if (cache.timestamp.secsTo(QDateTime::currentDateTime()) > CACHE_TTL_SECONDS)
    {
        m_detailsCache.remove(ipPort);
        return false;
    }
    players = cache.players;
    rules = cache.rules;
    return true;
}

void MainWindow::setCachedDetails(const QString &ipPort, const QStringList &players, const QMap<QString, QString> &rules)
{
    CachedDetails cache;
    cache.players = players;
    cache.rules = rules;
    cache.timestamp = QDateTime::currentDateTime();
    m_detailsCache[ipPort] = cache;
}

QProcess *MainWindow::createGslistProcess()
{
    QProcess *proc = new QProcess(this);
    proc->setProgram("gslist.exe");
    proc->setWorkingDirectory(QCoreApplication::applicationDirPath());
    return proc;
}