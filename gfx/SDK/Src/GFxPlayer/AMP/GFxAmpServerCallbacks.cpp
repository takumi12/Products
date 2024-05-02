/**********************************************************************

Filename    :   GFxAmpSendThreadCallback.cpp
Content     :   AMP server interface implementations
Created     :   January 2010
Authors     :   Alex Mantzaris

Copyright   :   (c) 2005-2010 Scaleform Corp. All Rights Reserved.

This file is provided AS IS with NO WARRANTY OF ANY KIND, INCLUDING
THE WARRANTY OF DESIGN, MERCHANTABILITY AND FITNESS FOR ANY PURPOSE.

**********************************************************************/

#include "GFxAmpServerCallbacks.h"
#include "GFxAmpServer.h"
#include "GFxAmpProfileFrame.h"
#include "GMsgFormat.h"

#ifndef GFX_AMP_SERVER

namespace { char dummyAmpServerCallbacksVar; }; // to disable warning LNK4221 on PC/Xbox

#else

// Called from the GFxAmpThreadManager send thread
// Handles messages on the received queue
bool GFxAmpSendThreadCallback::OnSendLoop()
{
    return GFxAmpServer::GetInstance().HandleNextMessage();
}

//////////////////////////////////////////////////////////////

// Constructor
GFxAmpStatusChangedCallback::GFxAmpStatusChangedCallback(GEvent* connectedEvent)
    : ConnectedEvent(connectedEvent)
{
}

// Called by GFxAmpThreadManager whenever a change in the connection status has been detected
void GFxAmpStatusChangedCallback::OnStatusChanged(StatusType newStatus, StatusType oldStatus, 
                                                  const char* message)
{
    GUNUSED(message);

    if (newStatus != oldStatus)
    {
        // Send the current paused/recording state to the player
        if (newStatus == CS_OK)
        {
            GFxAmpServer::GetInstance().SendAppControlCaps();
            GFxAmpServer::GetInstance().SendCurrentState();
        }

        // Signal the server that connection has been established
        // This is used so that the application can wait for a connection
        // during startup, and thus can get profiled from the first frame
        if (ConnectedEvent != NULL)
        {
            if (newStatus == CS_OK)
            {
                ConnectedEvent->SetEvent();
            }
            else
            {
                ConnectedEvent->ResetEvent();
            }
        }
    }
}

void GFxAmpStatusChangedCallback::OnMsgVersionMismatch(int otherVersion)
{
    GString logMsg;
    G_Format(logMsg, "AMP message version mismatch (Server {0}, Client {1}) - full functionality may not be available",
        GFxAmpMessage::GetLatestVersion(), otherVersion);
    GFxAmpServer::GetInstance().SendLog(logMsg, static_cast<int>(logMsg.GetLength()), GFxLogConstants::Log_MessageType_Message);
}

void GFxAmpStatusChangedCallback::OnMsgGFxVersionChanged(GFxAmpMessage::GFxVersionType newVersion)
{
    GUNUSED(newVersion); // AMP server doesn't care what is the GFx version of the client
}

#endif   // GFX_AMP_SERVER
