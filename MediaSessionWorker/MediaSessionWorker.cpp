#include "MediaSessionWorker.h"
#include <windows.h>
#include <winrt/Windows.Foundation.h>
#include <winrt/Windows.Media.Control.h>
#include <winrt/Windows.System.h>
#include <QDebug>
#include <stdexcept>

#pragma comment(lib, "runtimeobject.lib")

MediaSessionWorker::MediaSessionWorker(QObject* parent)
    : QObject(parent)
{
}

void MediaSessionWorker::process()
{
    try
    {
        // Initialize COM in a single-threaded apartment (STA)
        winrt::init_apartment(winrt::apartment_type::single_threaded);

        // Start asynchronous media session processing
        auto task = winrt::Windows::Media::Control::GlobalSystemMediaTransportControlsSessionManager::RequestAsync();

        // Use a completion handler to process the result
        task.Completed([this](winrt::Windows::Foundation::IAsyncOperation<winrt::Windows::Media::Control::GlobalSystemMediaTransportControlsSessionManager> const& sender, winrt::Windows::Foundation::AsyncStatus status)
                       {
                           if (status == winrt::Windows::Foundation::AsyncStatus::Completed)
                           {
                               try
                               {
                                   auto sessionManager = sender.GetResults();
                                   auto currentSession = sessionManager.GetCurrentSession();

                                   if (currentSession)
                                   {
                                       MediaSession session;

                                       // Retrieve media properties from the session
                                       auto mediaProperties = currentSession.TryGetMediaPropertiesAsync().get();
                                       session.title = QString::fromStdWString(mediaProperties.Title().c_str());
                                       session.artist = QString::fromStdWString(mediaProperties.Artist().c_str());

                                       // Get playback control information
                                       auto playbackInfo = currentSession.GetPlaybackInfo();
                                       auto playbackControls = playbackInfo.Controls();
                                       session.canGoNext = playbackControls.IsNextEnabled();
                                       session.canGoPrevious = playbackControls.IsPreviousEnabled();

                                       // Get the playback state (Playing, Paused, Stopped)
                                       auto playbackStatus = playbackInfo.PlaybackStatus();

                                       switch (playbackStatus)
                                       {
                                       case winrt::Windows::Media::Control::GlobalSystemMediaTransportControlsSessionPlaybackStatus::Playing:
                                           session.playbackState = "Playing";
                                           break;
                                       case winrt::Windows::Media::Control::GlobalSystemMediaTransportControlsSessionPlaybackStatus::Paused:
                                           session.playbackState = "Paused";
                                           break;
                                       case winrt::Windows::Media::Control::GlobalSystemMediaTransportControlsSessionPlaybackStatus::Stopped:
                                           session.playbackState = "Stopped";
                                           break;
                                       default:
                                           session.playbackState = "Unknown";
                                           break;
                                       }

                                       emit sessionReady(session);  // Send session data to main thread
                                   }
                                   else
                                   {
                                       emit sessionError("No active media session found.");
                                   }
                               }
                               catch (const std::exception& ex)
                               {
                                   emit sessionError(QString("Error during session handling: %1").arg(ex.what()));
                               }
                           }
                           else
                           {
                               emit sessionError("Failed to request media session.");
                           }
                       });

        // Do not wait manually, the callback will handle the result
        // Uninitialize COM when done
        // winrt::uninit_apartment() will now be handled when `process()` exits

    }
    catch (const std::exception& ex)
    {
        emit sessionError(QString("Error initializing COM: %1").arg(ex.what()));
    }
}
