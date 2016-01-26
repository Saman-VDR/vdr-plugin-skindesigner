#define __STL_CONFIG_H
#include "displayreplay.h"

cSDDisplayReplay::cSDDisplayReplay(cViewReplay *replayView, bool ModeOnly) {
    init = true;
    view = replayView;
    ok = view->Init();
    if (!ok)
        esyslog("skindesigner: Error initiating displayreplay view - aborting");
    view->SetModeOnly(ModeOnly);
}

cSDDisplayReplay::~cSDDisplayReplay() {
    view->Close();
}

void cSDDisplayReplay::SetRecording(const cRecording *Recording) {
    if (ok) {
        view->SetRecording(Recording);
        if (init) {
            view->SetRecordingLength(Recording->LengthInSeconds());
            SetTimeShiftValues(Recording);
            init = false;
        }
    }
}

void cSDDisplayReplay::SetTitle(const char *Title) {
    view->SetTitle(Title);
}

void cSDDisplayReplay::SetMode(bool Play, bool Forward, int Speed) {
    if (!ok)
        return;
    if (!Play && Speed < 0) {
        cControl *control = cControl::Control();
        if (control) {
            const cRecording *recording = control->GetRecording();
            if (recording && recording->FileName()) {
                view->StartOnPause(recording->FileName());
            }
        }
    } else {
        view->ClearOnPause();
    }

    view->SetControlIcons(Play, Forward, Speed);
}

void cSDDisplayReplay::SetProgress(int Current, int Total) {
    if (ok) {
        view->SetProgressbar(Current, Total);
        view->SetMarks(marks, Current, Total);
        view->SetEndTime(Current, Total);
    }
}

void cSDDisplayReplay::SetCurrent(const char *Current) {
    if (ok)
        view->SetCurrent(Current);
}

void cSDDisplayReplay::SetTotal(const char *Total) {
    if (ok)
        view->SetTotal(Total);
}

void cSDDisplayReplay::SetJump(const char *Jump) {
    if (ok)
        view->SetJump(Jump);
}

void cSDDisplayReplay::SetMessage(eMessageType Type, const char *Text) {
    if (ok)
        view->SetMessage(Type, Text);
}

void cSDDisplayReplay::Flush(void) {
    if (!ok)
        return;
    view->Flush();
}

void cSDDisplayReplay::SetTimeShiftValues(const cRecording *recording) {
    bool isTimeShift = false;
#if APIVERSNUM >= 20101
    int usage = recording->IsInUse();
    if (usage & ruTimer)
        isTimeShift = true;
#endif
    if (!isTimeShift)
        return;
    const cRecordingInfo *recInfo = recording->Info();
    if (!recInfo)
        return;
    const cEvent *event = recInfo->GetEvent();
    if (!event)
        return;
    double fps = recording->FramesPerSecond();
    time_t liveEventStop = event->EndTime();
    time_t recordingStart = time(0) - recording->LengthInSeconds();
    int framesTotal = (liveEventStop - recordingStart)*fps;
    int recLength = liveEventStop - recordingStart;
    view->SetTimeShift(framesTotal, recLength);
}
