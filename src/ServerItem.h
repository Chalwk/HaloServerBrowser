// Copyright (c) 2026 Jericho Crosby (Chalwk).
// Licensed under the GPL License.

#pragma once

#include <QString>

struct ServerItem
{
    QString ipPort;
    QString ip;
    QString port;
    QString hostname;
    QString mapname;
    QString gametype;
    QString gamevariant;
    int numplayers = 0;
    int maxplayers = 0;
    bool password = false;
    int ping = 999;
};