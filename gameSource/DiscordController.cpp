#include <cstdio>
#include <cassert>
#include <cstring>
#include <climits>
#include <ctime>

#include "minorGems/util/SettingsManager.h"
#include "minorGems/util/stringUtils.h"
#include "minorGems/system/Thread.h"

#include "objectBank.h"
#include "LivingLifePage.h"
#include "DiscordController.h"
#include "emotion.h"

#include <time.h>

//#define DISCORD_DEBUGGING
#ifdef DISCORD_DEBUGGING
// print to stdout then flush it.
#define VERBOSE(fmt, ...)                                \
    printf("[VERBOSE] " fmt __VA_OPT__(, ) __VA_ARGS__); \
    fflush(stdout)
#else
// Verbose is currently disabled enable it by uncommenting DISCORD_DEBUGGING definition or add -DDISCORD_DEBUGGING to the compiler flags
#define VERBOSE(fmt, ...)
#endif

// from game.cpp, used to check if current server connection request is a new game request or a reconnect attempt.
extern char userReconnect;

char dDisplayGame = false;      // initial value overriden in setttings
char dDisplayStatus = false;    // initial value overriden in setttings
char dDisplayDetails = false;   // initial value overriden in setttings
char dDisplayFirstName = false; // initial value overriden in setttings

// last time we tried to connect to the discord app
time_t lastReconnectAttempt = 0;
time_t inited_at = 0;

// last reported name to the discord app
char *dLastReportedName = NULL;
// last reported age to the discord app
int dLastDisplayedAge = -1;
// last reported fertility status to the discord app
char dLastWasInfertile = false;
// did we previously report AFK=true status to the discord app
char dLastWasIdle = false;
// did we previously report status to the discord app?
char dLastDisplayStatus = false;
// did we previously report details to the discord app?
char dLastDisplayDetails = false;
// did we previously report our first name to the discord app?
char dLastDisplayFirstName = false;
// did we report anything previously?, this is set to false whenever the discord
// app is re-connectes after a disconnect or when the user re-enables the discord option from the setting
// after they disabled it.
char dFirstReportDone = false;

// was the loaded key invalid in it's format?
char invalidKey = false;

int numInstances = 0;
int reconnectAttemptDelaySeconds = 10;

// value is from server settings afkEmotionIndex.ini, used to report AFK status
static int afkEmotionIndex = 21;

void DISCORD_CALLBACK OnActivityUpdate(void *data, EDiscordResult result)
{
    VERBOSE("OnActivityUpdate() %d was returned\n", result);
    if (result != EDiscordResult::DiscordResult_Ok)
    {
        printf("discord error OnActivityUpdate(): activity update failed result %d was returned\n", (int)result);
    }
    else
        invalidKey = false; // we thought key was invalid in it's format, but it did the job anyways...
}

DiscordController::DiscordController()
{
    memset(&app, 0, sizeof(app));
    // all defaults to more privacy(if not settings files exists).
    dDisplayGame = SettingsManager::getIntSetting("discordRichPresence", 1) > 0;
    dDisplayStatus = SettingsManager::getIntSetting("discordRichPresenceStatus", 0) > 0;
    dDisplayDetails = SettingsManager::getIntSetting("discordRichPresenceDetails", 0) > 0;
    dDisplayFirstName = SettingsManager::getIntSetting("discordRichPresenceHideFirstName", 1) <= 0;

    printf("discord: discordRichPresence.ini=%d\n", dDisplayGame);
    printf("discord: discordRichPresenceStatus.ini=%d\n", dDisplayStatus);
    printf("discord: discordRichPresenceDetails.ini=%d\n", dDisplayDetails);
    printf("discord: discordRichPresenceHideFirstName.ini=%d\n", !dDisplayFirstName);

    discordControllerInstance = this; // this is extern used in SettingsPage.cpp
    numInstances++;
    if (numInstances > 1)
    {
        // makes sure that discordControllerInstance is set only once
        printf("discord warning: more than one instance is active!, discordControllerInstance was changed\n");
    }
    inited_at = time(0);
}

// asyncronously attempt connection
EDiscordResult DiscordController::connect()
{
    VERBOSE("DiscordController::connect() was called\n");
    if (invalidKey)
    {
        return EDiscordResult::DiscordResult_InvalidSecret;
    }
    if (!dDisplayGame)
    {
        printf("discord warning connect(): setting dDisplayGame is false(discordRichPresence.ini=0), will not attempt to connect, we will return %d to our caller\n", EDiscordResult::DiscordResult_ApplicationMismatch);
        return EDiscordResult::DiscordResult_ApplicationMismatch;
    }
    struct IDiscordActivityEvents activities_events;
    memset(&activities_events, 0, sizeof(activities_events));

    struct DiscordCreateParams params;
    DiscordCreateParamsSetDefault(&params);

    char *discord_client_id = SettingsManager::getStringSetting("discord_client_id", "1071527161049124914");
    VERBOSE("discord_client_id to parse: %s\n", discord_client_id);
    // TODO: what happens if discord change the key to accept letters too?
    char *endptr;
    DiscordClientId parsed_client_id = strtoll(discord_client_id, &endptr, 10);
    VERBOSE("discord_client_id after parse: %lld\n", parsed_client_id);
    if (*endptr != '\0')
    {
        VERBOSE("discord_client_id key was not fully read from string, key is probably invalid\n");
        invalidKey = true;
        // return EDiscordResult::DiscordResult_InvalidSecret; // <--- let it try one time only, if it succeeds in updating activity it will be set to false in the update activity callback
    }
    delete[] discord_client_id;
    // delete[] endptr;

    if (parsed_client_id == LONG_MAX || parsed_client_id == LONG_MIN || !parsed_client_id)
    {
        printf("discord error connect(): failed to parse discord_client_id setting\n");
        return EDiscordResult::DiscordResult_InternalError;
    }
    params.client_id = parsed_client_id;
    params.flags = EDiscordCreateFlags::DiscordCreateFlags_NoRequireDiscord;
    params.event_data = &app;
    params.activity_events = &activities_events;
    EDiscordResult result = DiscordCreate(DISCORD_VERSION, &params, &app.core);
    if (result != EDiscordResult::DiscordResult_Ok)
    {
        printf("discord error connect(): failed to connect to discord client, result returned: %d\n", (int)result);
        memset(&app, 0, sizeof(app));
        return result;
    }

    app.activities = app.core->get_activity_manager(app.core);
    app.application = app.core->get_application_manager(app.core);

    memset(&activity, 0, sizeof(activity));
    // TOOD: what happens if wrong image name was given?
    char *gameLargeIcon = SettingsManager::getStringSetting("discordGameLargeIcon", "icon");
    strncpy(activity.assets.large_image, gameLargeIcon, sizeof(activity.assets.large_image));
    delete[] gameLargeIcon;

    char *gameShortDescrtiption = SettingsManager::getStringSetting("discordGameShortDescription", "A multiplayer survival game of parenting and civilization building, Join us on discord to play!");
    strncpy(activity.assets.large_text, gameShortDescrtiption, sizeof(activity.assets.large_text));
    delete[] gameShortDescrtiption;

    activity.timestamps.start = inited_at;

    // force show "Playing a game" because updateActivity will not be called if discordRichPresenceDetails.ini=0
    app.activities->update_activity(app.activities, &activity, NULL, OnActivityUpdate);
    result = app.core->run_callbacks(app.core);
    if (result != EDiscordResult::DiscordResult_Ok)
    {
        printf("discord error connect(): first attempt to run_callbacks() failed code resturned: %d\n", result);
        return result;
    }
    if (invalidKey)
        printf("discord connect(): connectd with a probaly invalid key\n");
    else
        printf("discord connect(): connectd\n");
    dFirstReportDone = false;
    // lastReconnectAttempt = 0;
    isHealthy = true;
    return result;
}

// set the activity data for the next call of core->run_callbacks()
void DiscordController::updateActivity(ActivityType activity_type, const char *details, const char *state)
{
    VERBOSE("DiscordController::updateActivity(%d, %s, %s) was called\n", activity_type, details, state);
    if (!dDisplayGame)
    {
        VERBOSE("DiscordController::updateActivity(%d, %s, %s) early return because dDisplayGame is false\n", activity_type, details, state);
        return;
    }

    // update it even if !dDisplayStatus to remove the previous status.
    //! if (!dDisplayStatus)
    //! {
    //!     return ....
    //! }
    printf("discord: updateActivity details=\"%s\" and state=\"%s\"\n", details, state);
    if (!isConnected())
    {
        printf("discord error updateActivity(): we are not connected, this call will be skipped\n");
        return;
    }

    if (details == NULL && state == NULL)
    {
        printf("discord warning updateActivity(): both state and details are NULL\n");
    }

    if (details != NULL)
    {
        strncpy(activity.details, details, sizeof(activity.details));
    }
    if (state != NULL)
    {
        if (!dDisplayDetails)
        {
            VERBOSE("DiscordController::updateActivity(%d, %s, %s) details will be masked because dDisplayDetails is false\n", activity_type, details, state);
            strncpy(activity.state, "", sizeof(activity.details));
        }
        else
        {
            strncpy(activity.state, state, sizeof(activity.details));
        }
    }
    dFirstReportDone = true;
    dLastDisplayDetails = dDisplayDetails;
    dLastDisplayStatus = dDisplayStatus;
    dLastDisplayFirstName = dDisplayFirstName;
    app.current_activity = activity_type;

    app.activities->update_activity(app.activities, &activity, NULL, OnActivityUpdate);
}

void DiscordController::disconnect()
{
    VERBOSE("DiscordController::disconnect() was called\n");
    if (isHealthy)
    {
        isHealthy = false;
        app.core->destroy(app.core);
        memset(&app, 0, sizeof(app));
        printf("discord: disconnect() called, app core destroyed\n");
    }
}

DiscordController::~DiscordController()
{
    VERBOSE("DiscordController::~DiscordController() was called\n");
    disconnect();
    numInstances--;
    discordControllerInstance = NULL;
}

EDiscordResult DiscordController::runCallbacks()
{
    VERBOSE("DiscordController::runCallbacks() was called\n");
    if (!dDisplayGame)
    {
        VERBOSE("DiscordController::runCallbacks() early return EDiscordResult::DiscordResult_ApplicationMismatch(%d) because dDisplayGame is false\n",
                EDiscordResult::DiscordResult_ApplicationMismatch);
        return EDiscordResult::DiscordResult_ApplicationMismatch;
    }
    if (!app.core || !isConnected())
    {
        VERBOSE("DiscordController::runCallbacks(): core not initialized, will try reconnect or skip this call\n");
        if (invalidKey)
        {
            VERBOSE("DiscordController::runCallbacks() core not initialized because invalid is key\n");
            return EDiscordResult::DiscordResult_ApplicationMismatch;
        }
        time_t now = time(0);
        if (now - lastReconnectAttempt > reconnectAttemptDelaySeconds)
        {
            lastReconnectAttempt = now;
            printf("discord runCallbacks(): not connected attempting reconnect now, then will skip this call\n");
            connect(); // TODO: does this block while connecting?
        }
        return EDiscordResult::DiscordResult_ApplicationMismatch;
    }
    EDiscordResult result = app.core->run_callbacks(app.core);
    if (result != EDiscordResult::DiscordResult_Ok)
    {
        VERBOSE("DiscordController::runCallbacks(): failed to run callbacks loop, result from run_callbacks(): %d, connection marked unhealthy\n", (int)result);
        isHealthy = false;
        if (result == EDiscordResult::DiscordResult_NotRunning)
            printf("discord error runCallbacks(): discord app not running, connection marked unhealthy, we may attempt connection again after %d seconds.\n", reconnectAttemptDelaySeconds);
    }
    return result;
}

bool DiscordController::isConnected()
{
    return isHealthy;
}

ActivityType DiscordController::getCurrentActivity()
{
    return app.current_activity;
}

// send new activity to the discord application (only if needed).
// this is done by comparing our previous RichPresnece status.
void DiscordController::lazyUpdateRichPresence(DiscordCurrentGamePage page, GamePage *dataPage)
{
    VERBOSE("DiscordController::lazyUpdateRichPresence(%d, %p) was called current activity is %d\n", page, dataPage, getCurrentActivity());
    if (!dDisplayGame || !dDisplayStatus)
    {
#ifdef DISCORD_DEBUGGING
        if (!dDisplayGame)
        {
            VERBOSE("DiscordController::lazyUpdateRichPresence(%d, %p) early return because dDisplayGame is false\n", page, dataPage);
        }
        else
        {
            VERBOSE("DiscordController::lazyUpdateRichPresence(%d, %p) early return because dDisplayStatus is false\n", page, dataPage);
        }
#endif // ENABLE_VERBOSE
        return;
    }
    if (!app.core || !isConnected())
    {
        VERBOSE("DiscordController::lazyUpdateRichPresence(%d, %p) core not initialized or we are not connected, will try connect if (now - lastReconnectAttempt > 10)\n", page, dataPage);
        if (invalidKey)
        {
            VERBOSE("DiscordController::lazyUpdateRichPresence(%d, %p) invalid key\n", page, dataPage);
            return;
        }
        time_t now = time(0);
        if (now - lastReconnectAttempt > 10)
        {
            lastReconnectAttempt = now;
            printf("discord lazyUpdateRichPresence(): not connected attempting reconnect now, then will skip this call\n");
            connect(); // TODO: does this block while connecting?
        }
        return;
    }

    if (page == DiscordCurrentGamePage::LIVING_LIFE_PAGE)
    {
        if (dataPage == NULL)
        {
            printf("discord error lazyUpdateRichPresence(): dataPage is NULL sig fault imminent.\n");
            fflush(stdout);
        }
        LivingLifePage *livingLifePage = (LivingLifePage *)dataPage;
        LiveObject *ourObject = livingLifePage->getOurLiveObject();
        if (ourObject != NULL)
        {
            char *ourName;
            int ourAge = (int)livingLifePage->getLastComputedAge();
            if (ourObject->name != NULL)
                ourName = autoSprintf("%s", ourObject->name);
            else // "NAMELESS" is also used as a key in dDisplayFirstName below, if changed also change it there...
                ourName = stringDuplicate("NAMELESS");
            // TODO: not necesarrly that when we have afkEmote means we are really idle!, 2hol allows you to set the afk emotion with /sleep
            char isIdle = ourObject->currentEmot != NULL && getEmotion(afkEmotionIndex) == ourObject->currentEmot;
            char infertileFound, fertileFound;
            char *t1, *t2; // temp swap strings

            t1 = replaceOnce(ourName, "+INFERTILE+", "", &infertileFound);
            delete[] ourName;
            if (dLastDisplayFirstName != dDisplayFirstName || dLastDisplayDetails != dDisplayDetails || !dFirstReportDone || dDisplayStatus != dLastDisplayStatus || ActivityType::LIVING_LIFE != getCurrentActivity() || dLastReportedName == NULL || 0 != strcmp(dLastReportedName, ourName) || dLastDisplayedAge != ourAge || dLastWasInfertile != infertileFound || dLastWasIdle != isIdle)
            {
                t2 = replaceOnce(t1, "+FERTILE+", "", &fertileFound);
                delete[] t1;
                t1 = trimWhitespace(t2);
                delete[] t2;
                ourName = stringDuplicate(strlen(t1) == 0 ? "NAMELESS" : t1);
                delete[] t1;
                if (dLastReportedName != NULL)
                    delete[] dLastReportedName;
                dLastReportedName = stringDuplicate(ourName);
                dLastDisplayedAge = ourAge;
                dLastWasInfertile = infertileFound;
                dLastWasIdle = isIdle;
                const char ourGender = getObject(ourObject->displayID)->male ? 'M' : 'F';
                char *details = autoSprintf("Living Life, Age %d [%c]%s%s", ourAge, ourGender, infertileFound ? (char *)" [INF]" : (char *)"", isIdle ? (char *)" [IDLE]" : (char *)"");
                char *state;
                if (!dDisplayFirstName)
                {
                    // hide the first name or the EVE mark.
                    char firstName[99];
                    char lastName[99];
                    int numNames = sscanf(ourName, "%99s %99s", firstName, lastName);
                    if (numNames > 1)
                    {
                        state = autoSprintf("In %s's Family", lastName);
                    }
                    else
                    {
                        // TODO: when does this happen, other instances except NONAME may need to be handled differrently...
                        state = stringDuplicate(ourName);
                    }
                }
                else
                {
                    state = autoSprintf("As %s", ourName);
                }
                updateActivity(ActivityType::LIVING_LIFE, details, state);
                delete[] details;
                delete[] state;
                delete[] ourName;
            }
            else
            {
                delete[] t1;
            }
        }
    }
    else if (page == DiscordCurrentGamePage::DISONNECTED_PAGE)
    {
        if (!dFirstReportDone || dDisplayStatus != dLastDisplayStatus || ActivityType::DISCONNECTED != getCurrentActivity())
            updateActivity(ActivityType::DISCONNECTED, "DISCONNECTED!", "");
    }
    else if (page == DiscordCurrentGamePage::DEATH_PAGE)
    {
        if (dLastDisplayDetails != dDisplayDetails || !dFirstReportDone || dDisplayStatus != dLastDisplayStatus || ActivityType::DEATH_SCREEN != getCurrentActivity())
        {
            // TODO: show death reson? (when dDisplayDetails)
            if (dataPage == NULL)
            {
                printf("discord error lazyUpdateRichPresence(): dataPage is NULL sig fault imminent.\n");
                fflush(stdout);
            }
            LivingLifePage *livingLifePage = (LivingLifePage *)dataPage;
            LiveObject *ourObject = livingLifePage->getOurLiveObject();
            if (ourObject != NULL)
            {
                char *ourName;
                int ourAge = (int)livingLifePage->getLastComputedAge();
                if (ourObject->name != NULL)
                    ourName = autoSprintf("%s", ourObject->name);
                else
                    ourName = stringDuplicate("NAMELESS");
                delete[] dLastReportedName;
                dLastReportedName = stringDuplicate(ourName);
                dLastDisplayedAge = ourAge;

                char infertileFound, fertileFound;
                char *t1, *t2; // temp swap strings

                t1 = replaceOnce(ourName, "+FERTILE+", "", &fertileFound);
                delete[] ourName;
                t2 = replaceOnce(t1, "+INFERTILE+", "", &infertileFound);
                delete[] t1;
                t1 = trimWhitespace(t2);
                delete[] t2;
                ourName = stringDuplicate(strlen(t1) == 0 ? "NAMELESS" : t1);
                delete[] t1;
                const char ourGender = getObject(ourObject->displayID)->male ? 'M' : 'F';
                char *details = autoSprintf("Died at age %d [%c]%s", ourAge, ourGender);
                char *state;
                if (!dDisplayFirstName)
                {
                    // hide the first name or the EVE mark.
                    char firstName[99];
                    char lastName[99];
                    int numNames = sscanf(ourName, "%99s %99s", firstName, lastName);
                    if (numNames > 1)
                    {
                        state = autoSprintf("In %s's Family", lastName);
                    }
                    else
                    {
                        // TODO: when does this happend, other instances except NONAME...
                        state = stringDuplicate(ourName);
                    }
                }
                else
                {
                    state = autoSprintf("As %s", ourName);
                }
                updateActivity(ActivityType::DEATH_SCREEN, details, state);
                dLastReportedName = stringDuplicate(ourName);
                delete[] details;
                delete[] state;
                delete[] ourName;
            }
            else
            {
                updateActivity(ActivityType::DEATH_SCREEN, "Died", "");
            }
        }
    }
    else if (page == DiscordCurrentGamePage::LOADING_PAGE)
    {
        if (!dFirstReportDone || dDisplayStatus != dLastDisplayStatus || ActivityType::GAME_LOADING != getCurrentActivity())
            updateActivity(ActivityType::GAME_LOADING, "Loading Game...", "");
    }
    else if (page == DiscordCurrentGamePage::WAITING_TO_BE_BORN_PAGE)
    {
        if (!dFirstReportDone || dDisplayStatus != dLastDisplayStatus || ActivityType::WAITING_TO_BE_BORN != getCurrentActivity())
        {
            if (userReconnect)
                updateActivity(ActivityType::WAITING_TO_BE_BORN, "Reconnecting!...", "");
            else
                updateActivity(ActivityType::WAITING_TO_BE_BORN, "Waiting to be born...", "");
        }
    }
    else if (page == DiscordCurrentGamePage::MAIN_MENU_PAGE)
    {
        if (!dFirstReportDone || dDisplayStatus != dLastDisplayStatus || ActivityType::IN_MAIN_MENU != getCurrentActivity())
            updateActivity(ActivityType::IN_MAIN_MENU, "In Main Menu", "");
    }
    else if (page == DiscordCurrentGamePage::SETTINGS_PAGE)
    {
        if (!dFirstReportDone || dDisplayStatus != dLastDisplayStatus || ActivityType::EDITING_SETTINGS != getCurrentActivity())
            updateActivity(ActivityType::EDITING_SETTINGS, "Editing Settings", "");
    }
    else if (page == DiscordCurrentGamePage::LIVING_TUTORIAL_PAGE)
    {
        if (dataPage == NULL)
        {
            printf("discord error lazyUpdateRichPresence(): dataPage is NULL sig fault imminent.\n");
            fflush(stdout);
        }
        LivingLifePage *livingLifePage = (LivingLifePage *)dataPage;
        LiveObject *ourObject = livingLifePage->getOurLiveObject();
        char isIdle = false;
        if (ourObject != NULL)
        {
            isIdle = ourObject->currentEmot != NULL && getEmotion(afkEmotionIndex) == ourObject->currentEmot;
        }
        if (dLastWasIdle != isIdle || !dFirstReportDone || dDisplayStatus != dLastDisplayStatus || ActivityType::PLAYING_TUTORIAL != getCurrentActivity())
        {
            dLastWasIdle = isIdle;
            updateActivity(ActivityType::PLAYING_TUTORIAL, "Playing Tutorial", isIdle ? "[IDLE]" : "");
        }
    }
    else if (page == DiscordCurrentGamePage::CONNECTION_LOST_PAGE)
    {
        if (!dFirstReportDone || dDisplayStatus != dLastDisplayStatus || ActivityType::CONNECTION_LOST != getCurrentActivity())
            updateActivity(ActivityType::CONNECTION_LOST, "DISCONNECTED!", "");
    }
    else
    {
        VERBOSE("discord error lazyUpdateRichPresence(): unhandled DiscordCurrentGamePage parameter value %d. nothing will be updated\n", page);
    }
}
void DiscordController::updateDisplayGame(char newValue)
{
    printf("discord: DisplayGame was changed to %d\n", newValue);
    dDisplayGame = newValue;
    if (!newValue)
        disconnect();
    else
        connect(); // TODO: does this block while connecting?
}
void DiscordController::updateDisplayStatus(char newValue)
{
    printf("discord: DisplayStatus was changed to %d\n", newValue);
    if (!newValue)
    {
        updateActivity(ActivityType::NO_ACTIVITY, "", "");
        dLastDisplayStatus = newValue;
    }
    dDisplayStatus = newValue;
}
void DiscordController::updateDisplayDetails(char newValue)
{
    printf("discord: DisplayDetails was changed to %d\n", newValue);
    dDisplayDetails = newValue;
}
void DiscordController::updateDisplayFirstName(char newValue)
{
    printf("discord: DisplayFirstName was changed to %d\n", newValue);
    dDisplayFirstName = newValue;
}