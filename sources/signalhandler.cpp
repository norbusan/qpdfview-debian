/*

Copyright 2012 Adam Reichold

This file is part of qpdfview.

qpdfview is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 2 of the License, or
(at your option) any later version.

qpdfview is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with qpdfview.  If not, see <http://www.gnu.org/licenses/>.

*/

#include "signalhandler.h"

#include <signal.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>

#include <QSocketNotifier>

int SignalHandler::s_sigintSockets[2];
int SignalHandler::s_sigtermSockets[2];

bool SignalHandler::prepare()
{
    if(socketpair(AF_UNIX, SOCK_STREAM, 0, s_sigintSockets) != 0)
    {
        return false;
    }

    if(socketpair(AF_UNIX, SOCK_STREAM, 0, s_sigtermSockets) != 0)
    {
        return false;
    }

    struct sigaction sigintAction;

    sigintAction.sa_handler = SignalHandler::sigintHandler;
    sigemptyset(&sigintAction.sa_mask);
    sigintAction.sa_flags = SA_RESTART;

    if(sigaction(SIGINT, &sigintAction, 0) != 0)
    {
        return false;
    }

    struct sigaction sigtermAction;

    sigtermAction.sa_handler = SignalHandler::sigtermHandler;
    sigemptyset(&sigtermAction.sa_mask);
    sigtermAction.sa_flags = SA_RESTART;

    if(sigaction(SIGTERM, &sigtermAction, 0) != 0)
    {
        return false;
    }

    return true;
}

SignalHandler::SignalHandler(QObject* parent) : QObject(parent),
    m_sigintNotifier(0),
    m_sigtermNotifier(0)
{
    m_sigintNotifier = new QSocketNotifier(s_sigintSockets[1], QSocketNotifier::Read, this);
    connect(m_sigintNotifier, SIGNAL(activated(int)), SLOT(on_sigint_activated()));

    m_sigtermNotifier = new QSocketNotifier(s_sigtermSockets[1], QSocketNotifier::Read, this);
    connect(m_sigtermNotifier, SIGNAL(activated(int)), SLOT(on_sigterm_activated()));
}

void SignalHandler::on_sigint_activated()
{
    m_sigintNotifier->setEnabled(false);

    char c;
    read(s_sigintSockets[1], &c, sizeof(c));

    emit sigintReceived();

    m_sigintNotifier->setEnabled(true);
}

void SignalHandler::on_sigterm_activated()
{
    m_sigtermNotifier->setEnabled(false);

    char c;
    read(s_sigtermSockets[1], &c, sizeof(c));

    emit sigtermReceived();

    m_sigtermNotifier->setEnabled(true);
}

void SignalHandler::sigintHandler(int)
{
    char c = 1;
    write(s_sigintSockets[0], &c, sizeof(c));
}

void SignalHandler::sigtermHandler(int)
{
    char c = 1;
    write(s_sigtermSockets[0], &c, sizeof(c));
}
