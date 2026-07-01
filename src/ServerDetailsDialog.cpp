// Copyright (c) 2026 Jericho Crosby (Chalwk).
// Licensed under the GPL License.

#include "ServerDetailsDialog.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QListWidget>
#include <QTableWidget>
#include <QHeaderView>
#include <QPushButton>
#include <QDialogButtonBox>

ServerDetailsDialog::ServerDetailsDialog(const QString &ipPort,
                                         const QStringList &players,
                                         const QMap<QString, QString> &rules,
                                         bool showPlayers,
                                         QWidget *parent)
    : QDialog(parent)
{
    setupUI(ipPort, players, rules, showPlayers);
}

ServerDetailsDialog::~ServerDetailsDialog() {}

void ServerDetailsDialog::setupUI(const QString &ipPort,
                                  const QStringList &players,
                                  const QMap<QString, QString> &rules,
                                  bool showPlayers)
{
    setWindowTitle(showPlayers ? "Player List" : "Server Rules");
    resize(500, 400);

    QVBoxLayout *mainLayout = new QVBoxLayout(this);

    QLabel *titleLabel = new QLabel;
    if (showPlayers)
    {
        titleLabel->setText(QString("Players on %1").arg(ipPort));
    }
    else
    {
        titleLabel->setText(QString("Rules for %1").arg(ipPort));
    }
    titleLabel->setStyleSheet("font-weight: bold; font-size: 14px;");
    mainLayout->addWidget(titleLabel);

    if (showPlayers)
    {
        QListWidget *listWidget = new QListWidget;
        if (players.isEmpty())
        {
            listWidget->addItem("No players currently on this server.");
        }
        else
        {
            for (const QString &p : players)
                listWidget->addItem(p);
        }
        mainLayout->addWidget(listWidget);
    }
    else
    {
        QTableWidget *table = new QTableWidget;
        table->setColumnCount(2);
        table->setHorizontalHeaderLabels({"Key", "Value"});
        table->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
        if (rules.isEmpty())
        {
            table->setRowCount(1);
            table->setItem(0, 0, new QTableWidgetItem("No additional rules available."));
            table->setSpan(0, 0, 1, 2);
        }
        else
        {
            table->setRowCount(rules.size());
            int row = 0;
            for (auto it = rules.begin(); it != rules.end(); ++it, ++row)
            {
                table->setItem(row, 0, new QTableWidgetItem(it.key()));
                table->setItem(row, 1, new QTableWidgetItem(it.value()));
            }
        }
        mainLayout->addWidget(table);
    }

    QDialogButtonBox *buttonBox = new QDialogButtonBox(QDialogButtonBox::Close);
    connect(buttonBox, &QDialogButtonBox::accepted, this, &QDialog::accept);
    connect(buttonBox, &QDialogButtonBox::rejected, this, &QDialog::reject);
    mainLayout->addWidget(buttonBox);
}