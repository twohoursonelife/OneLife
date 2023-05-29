#include <cstdio>
#include <cassert>
#include <cstring>
#include <climits>
#include <ctime>

#include "minorGems/util/SettingsManager.h"
#include "minorGems/util/stringUtils.h"

#include "objectBank.h"
#include "LivingLifePage.h"
#include "DiscordController.h"
#include "emotion.h"

#include <time.h>

// #define DISCORD_DEBUGGING // please disable when publishing, fflush() in VERBOSE will slow the execution
#ifdef DISCORD_DEBUGGING
// print verbose info to stdout then flush it. used for debugging
#define VERBOSE(fmt, ...)                                \
    printf("[VERBOSE] " fmt __VA_OPT__(, ) __VA_ARGS__); \
    fflush(stdout)
#else
// Verbose is currently disabled(this whole instruction will be empty string) enable it by uncommenting DISCORD_DEBUGGING definition above or add -DDISCORD_DEBUGGING to the compiler flags
#define VERBOSE(fmt, ...)
#endif

// from game.cpp, used to check if current server connection request is a new game request or a reconnect attempt.
extern char userReconnect;

// initial values will be overriden from setttings
char dDisplayGame = false;
char dDisplayStatus = false;
char dDisplayDetails = false;
char dDisplayFirstName = false;
char dDisplayAge = false;

time_t inited_at = 0;
// last time we tried to connect to the discord app
time_t lastReconnectAttempt = 0;

// how many seconds should we wait before retrying to connect to the discord client again?
int reconnectAttemptDelaySeconds = 10;

// last time we changed the activity data.
time_t lastStateSetAt = 0;

// we consider player is idle if his state did not change after this amount of seconds(excluding livingLife, it will check the afk emote).
time_t idleTime = 60 * 2;

// the last reported name size to the discord app
size_t dLastReportedNameLength = -1;

// last reported age to the discord app
int dPreviousAge; // initialized in connect()

// last reported fertility status to the discord app
char dLastWasInfertile = false;

// did we previously report AFK=true status to the discord app
char dLastWasIdle = false;

// previous value of DisplayStatus (after updateActivity)
char dLastDisplayStatus = false;

// previous value of DisplayDetails (after updateActivity)
char dLastDisplayDetails = false;

// previous value of DisplayFirstName (after updateActivity)
char dLastDisplayFirstName = false;

// previous value of DisplayAge (after updateActivity)
char dLastDisplayAge = false;

// is set to false if player re-enables discord rich presnece in settings or when game starts(here)
// set to true after updateActivity is called.
char dFirstReportDone = false;

// was the parsed discord key invalid in it's format?
char invalidKey = false;

int numInstances = 0;

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
    memset(&mApp, 0, sizeof(mApp));
    mIsHealthy = false;
    // all defaults to more privacy(if not settings files exists).
    dDisplayGame = SettingsManager::getIntSetting("discordRichPresence", 1) > 0;
    dDisplayStatus = SettingsManager::getIntSetting("discordRichPresenceStatus", 0) > 0;
    dDisplayDetails = SettingsManager::getIntSetting("discordRichPresenceDetails", 0) > 0;
    dDisplayFirstName = SettingsManager::getIntSetting("discordRichPresenceHideFirstName", 1) <= 0;
    dDisplayAge = SettingsManager::getIntSetting("discordRichPresenceShowAge", 1) > 0; // This setting is only changed via the .ini file, Setting page does not change it(too many options already).

    printf("discord: discordRichPresence.ini=%d\n", dDisplayGame);
    printf("discord: discordRichPresenceStatus.ini=%d\n", dDisplayStatus);
    printf("discord: discordRichPresenceDetails.ini=%d\n", dDisplayDetails);
    printf("discord: discordRichPresenceHideFirstName.ini=%d\n", !dDisplayFirstName);
    printf("discord: discordRichPresenceShowAge.ini=%d\n", dDisplayAge);

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
        VERBOSE("DiscordController::connect() skipped because previous connect() failure was due to invalidKey\n");
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

    char *discord_client_id = SettingsManager::getStringSetting("discord_client_id", "967932935569285160");
    VERBOSE("discord_client_id to parse: %s\n", discord_client_id);
    // TODO: what happens if discord change the key to accept letters too?
    char *endptr;
    DiscordClientId parsed_client_id = strtoll(discord_client_id, &endptr, 10);
    VERBOSE("discord_client_id after parse: %ld\n", parsed_client_id);
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
    params.event_data = &mApp;
    params.activity_events = &activities_events;
    EDiscordResult result = DiscordCreate(DISCORD_VERSION, &params, &mApp.core);
    if (result != EDiscordResult::DiscordResult_Ok)
    {
        printf("discord error connect(): failed to connect to discord client, result returned: %d\n", (int)result);
        memset(&mApp, 0, sizeof(mApp));
        return result;
    }

    mApp.activities = mApp.core->get_activity_manager(mApp.core);
    mApp.application = mApp.core->get_application_manager(mApp.core);

    memset(&mActivity, 0, sizeof(mActivity));
    // NOTE: if the image name does not exist on the application account no error is given.
    char *gameLargeIcon = SettingsManager::getStringSetting("discordGameLargeIcon", "icon-large");
    strncpy(mActivity.assets.large_image, gameLargeIcon, sizeof(mActivity.assets.large_image));
    delete[] gameLargeIcon;

    char *gameShortDescrtiption = SettingsManager::getStringSetting("discordGameShortDescription", "A multiplayer survival game of parenting and civilization building, Join us on discord to play!");
    strncpy(mActivity.assets.large_text, gameShortDescrtiption, sizeof(mActivity.assets.large_text));
    delete[] gameShortDescrtiption;

    mActivity.timestamps.start = inited_at;

    // force show "Playing a game" because updateActivity will not be called if discordRichPresenceDetails.ini=0
    mApp.activities->update_activity(mApp.activities, &mActivity, NULL, OnActivityUpdate);
    result = mApp.core->run_callbacks(mApp.core);
    if (result != EDiscordResult::DiscordResult_Ok)
    {
        if (result == EDiscordResult::DiscordResult_NotRunning)
        {
            printf("discord error connect(): first attempt to run_callbacks() failed because discord client is not running, we may attempt connecting again after %d seconds\n", reconnectAttemptDelaySeconds);
        }
        else
        {
            printf("discord error connect(): first attempt to run_callbacks() failed code resturned: %d\n", result);
        }
        return result;
    }
    if (invalidKey)
        printf("discord connect(): connectd with a probaly invalid key\n");
    else
        printf("discord connect(): connectd\n");
    dFirstReportDone = false;
    // lastReconnectAttempt = 0;
    mIsHealthy = true;
    dPreviousAge = -1;
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
        strncpy(mActivity.details, details, sizeof(mActivity.details));
    }
    if (state != NULL)
    {
        if (!dDisplayDetails)
        {
            VERBOSE("DiscordController::updateActivity(%d, %s, %s) details will be masked because dDisplayDetails is false\n", activity_type, details, state);
            strncpy(mActivity.state, "", sizeof(mActivity.details));
        }
        else
        {
            strncpy(mActivity.state, state, sizeof(mActivity.state));
        }
    }
    dFirstReportDone = true;
    dLastDisplayDetails = dDisplayDetails;
    dLastDisplayStatus = dDisplayStatus;
    dLastDisplayFirstName = dDisplayFirstName;
    dLastDisplayAge = dDisplayAge;
    mApp.current_activity = activity_type;

    mApp.activities->update_activity(mApp.activities, &mActivity, NULL, OnActivityUpdate);
}

void DiscordController::disconnect()
{
    VERBOSE("DiscordController::disconnect() was called\n");
    if (mIsHealthy)
    {
        if (mApp.core != NULL)
        {
            // IDiscordActivityManager *activity_manager = mApp.core->get_activity_manager(mApp.core);
            // if (activity_manager != NULL)
            // {
            //     activity_manager->clear_activity(activity_manager, NULL, NULL);
            //     mApp.core->run_callbacks(mApp.core);
            // }
            mApp.core->destroy(mApp.core);
        }
        else
        {
            printf("discord warning disconnect(): mIsHealthy=true while mApp.core is NULL\n"); // this is not normal!
        }
        mIsHealthy = false;
        memset(&mApp, 0, sizeof(mApp));
        printf("discord: disconnect() called, mApp core destroyed\n");
    }
}

DiscordController::~DiscordController()
{
    VERBOSE("DiscordController::~DiscordController() deconstructor was called\n");
    disconnect();
    numInstances--;
    discordControllerInstance = NULL;
}

// run pending activity changes asynchronously.
void DiscordController::runCallbacks()
{
    VERBOSE("DiscordController::runCallbacks() was called\n");
    if (!dDisplayGame)
    {
        VERBOSE("DiscordController::runCallbacks() early return because dDisplayGame is false\n");
        return;
    }
    if (!mApp.core || !isConnected())
    {
        VERBOSE("DiscordController::runCallbacks(): core not initialized, will try reconnect or skip this call\n");
        if (invalidKey)
        {
            VERBOSE("DiscordController::runCallbacks() core not initialized because invalid is key\n");
            return;
        }
        time_t now = time(0);
        if (now - lastReconnectAttempt > reconnectAttemptDelaySeconds)
        {
            lastReconnectAttempt = now;
            printf("discord runCallbacks(): not connected attempting reconnect now, then will skip this call\n");
            connect();
        }
        return;
    }
    EDiscordResult result = mApp.core->run_callbacks(mApp.core); // OnActivityUpdate will be asynchronously called when it's result is ready.
    if (result != EDiscordResult::DiscordResult_Ok)
    {
        VERBOSE("DiscordController::runCallbacks(): failed to run callbacks loop, result from run_callbacks(): %d, connection marked unhealthy\n", (int)result);
        mIsHealthy = false;
        if (result == EDiscordResult::DiscordResult_NotRunning)
            printf("discord error runCallbacks(): the discord client is not running, connection marked unhealthy, we may attempt connection again after %d seconds.\n", reconnectAttemptDelaySeconds);
    }
    VERBOSE("DiscordController::runCallbacks(): result returned: %d\n", (int)result);
}

char DiscordController::isConnected()
{
    return mIsHealthy;
}

ActivityType DiscordController::getCurrentActivity()
{
    return mApp.current_activity;
}

// send new activity to the discord application (only if needed).
// this is done by comparing our previous RichPresnece status.
void DiscordController::lazyUpdateRichPresence(DiscordCurrentGamePage page, GamePage *dataPage)
{
    // TODO: LivingLifePage should be decoupled from this controller, we may use this controller elsewhere, like gameEditor etc..
    // this function should be in game.cpp somehow
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
    if (!mApp.core || !isConnected())
    {
        VERBOSE("DiscordController::lazyUpdateRichPresence(%d, %p) core not initialized or we are not connected, will try connecting if (now - lastReconnectAttempt > %d)\n", page, dataPage, reconnectAttemptDelaySeconds);
        return;
    }

    if (page == DiscordCurrentGamePage::LIVING_LIFE_PAGE)
    {
        if (dataPage == NULL)
        {
            VERBOSE("discord error lazyUpdateRichPresence(): dataPage is NULL, early returning\n");
            return;
        }
        LivingLifePage *livingLifePage = (LivingLifePage *)dataPage;
        LiveObject *ourObject = livingLifePage->getOurLiveObject();
        if (ourObject == NULL)
        {
            VERBOSE("DiscordController::lazyUpdateRichPresence(%d, %p): early returning becauselivingLifePage->getOurLiveObject() returned null\n", page, dataPage);
            return;
        }
        char *ourName;
        int ourAge = (int)livingLifePage->getLastComputedAge();
        size_t ourNameLength;
        if (ourObject->name != NULL)
        {
            ourName = stringDuplicate(ourObject->name);
            ourNameLength = strlen(ourName);
        }
        else
        {
            // "NAMELESS" is also used as a key after dDisplayFirstName below, if changed also change it there...
            ourName = stringDuplicate("NAMELESS");
            ourNameLength = 0;
        }
        // TODO: not necesarrly that when we have afkEmote means we are really idle!, 2hol allows you to set the afk emotion with /sleep
        char isIdle = ourObject->currentEmot != NULL && getEmotion(afkEmotionIndex) == ourObject->currentEmot;
        char infertileFound, fertileFound;
        char *t1, *t2; // temp swap strings

        t1 = replaceOnce(ourName, "+INFERTILE+", "", &infertileFound);
        delete[] ourName;
        if (ActivityType::LIVING_LIFE != getCurrentActivity() || dLastDisplayFirstName != dDisplayFirstName || dLastDisplayDetails != dDisplayDetails || !dFirstReportDone || dDisplayStatus != dLastDisplayStatus || ourNameLength != dLastReportedNameLength || dDisplayAge != dLastDisplayAge || (dDisplayAge && dPreviousAge != ourAge) || dLastWasInfertile != infertileFound || dLastWasIdle != isIdle)
        {
            t2 = replaceOnce(t1, "+FERTILE+", "", &fertileFound);
            delete[] t1;
            t1 = trimWhitespace(t2);
            delete[] t2;
            ourName = stringDuplicate(strlen(t1) == 0 ? "NAMELESS" : t1); // "NAMELESS" is also used as a key after dDisplayFirstName below, if changed also change it there...
            delete[] t1;
            dLastWasInfertile = infertileFound;
            dLastWasIdle = isIdle;
            const char ourGender = getObject(ourObject->displayID)->male ? 'M' : 'F';
            char *agePart;
            if (dDisplayAge)
            {
                agePart = autoSprintf(" Age %d", ourAge);
                dPreviousAge = ourAge;
            }
            else
            {
                agePart = stringDuplicate("");
            }
            char *infertilePart = infertileFound ? (char *)"[INF] " : (char *)"";
            char *idlePart = isIdle ? (char *)" [IDLE]" : (char *)"";
            char *details = autoSprintf("Living Life, [%c]%s%s%s", ourGender, agePart, infertilePart, idlePart);
            delete[] agePart;
            char *state;
            if (!dDisplayFirstName)
            {
                // hide the first name or the EVE mark.
                char firstName[100];
                char lastName[100];
                int numNames = sscanf(ourName, "%99s %99s", firstName, lastName);
                if (numNames > 1)
                {
                    state = autoSprintf("In The %s Family", lastName);
                }
                else
                {
                    // TODO: when does this happen, other instances except NONAME may need to be handled differrently...
                    state = stringDuplicate(ourName);
                }
            }
            else if (0 != strcmp(ourName, "NAMELESS"))
            {
                state = autoSprintf("As %s", ourName);
            }
            else
            {
                state = stringDuplicate("NAMELESS");
            }
            updateActivity(ActivityType::LIVING_LIFE, details, state);
            dLastReportedNameLength = ourNameLength;
            delete[] details;
            delete[] state;
            delete[] ourName;
        }
        else
        {
            delete[] t1;
        }
    }
    else if (page == DiscordCurrentGamePage::DISONNECTED_PAGE)
    {
        char lastActivityDisconnectedPage = ActivityType::DISCONNECTED == getCurrentActivity();
        char isIdle = lastActivityDisconnectedPage && time(0) - lastStateSetAt > idleTime;
        if (dLastWasIdle != isIdle || !dFirstReportDone || dDisplayStatus != dLastDisplayStatus || !lastActivityDisconnectedPage) {
            if (!lastActivityDisconnectedPage)
            {
                lastStateSetAt = time(0);
            }
            char *details = autoSprintf("Disconnected!%s", (isIdle ? " [IDLE]" : ""));
            updateActivity(ActivityType::DISCONNECTED, details, "");
        }
        dLastWasIdle = isIdle;
    }
    else if (page == DiscordCurrentGamePage::DEATH_PAGE)
    {
        char lastActivityDeathPage = ActivityType::DEATH_SCREEN == getCurrentActivity();
        char isIdle = lastActivityDeathPage && time(0) - lastStateSetAt > idleTime;

        if (isIdle != dLastWasIdle || dLastDisplayDetails != dDisplayDetails || !dFirstReportDone || dDisplayStatus != dLastDisplayStatus || !lastActivityDeathPage)
        {
            if (!lastActivityDeathPage)
            {
                lastStateSetAt = time(0);
            }
            // TODO: show death reson? (when dDisplayDetails)
            if (dataPage == NULL)
            {
                VERBOSE("discord error lazyUpdateRichPresence(): dataPage is NULL, will early return.\n");
                return;
            }
            LivingLifePage *livingLifePage = (LivingLifePage *)dataPage;
            LiveObject *ourObject = livingLifePage->getOurLiveObject();
            if (ourObject != NULL)
            {
                char *ourName;
                int ourAge = (int)livingLifePage->getLastComputedAge();
                if (ourObject->name != NULL)
                    ourName = stringDuplicate(ourObject->name);
                else
                    ourName = stringDuplicate("NAMELESS");
                dPreviousAge = ourAge;

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
                // where they idle on on death?
                char wasIdle = ourObject->currentEmot != NULL && getEmotion(afkEmotionIndex) == ourObject->currentEmot;
                char *agePart;
                if (dDisplayAge)
                {
                    agePart = autoSprintf(" At Age %d", ourAge);
                    dPreviousAge = ourAge;
                }
                else
                {
                    agePart = stringDuplicate("");
                }
                char *idlePart = wasIdle ? (char *)"[IDLE(on death)]" : (char *)"";
                char *details = autoSprintf("Died%s [%c]%s", agePart, ourGender, idlePart);
                delete[] agePart;
                char *state;
                if (!dDisplayFirstName)
                {
                    // hide the first name or the EVE mark.
                    char firstName[100];
                    char lastName[100];
                    int numNames = sscanf(ourName, "%99s %99s", firstName, lastName);
                    if (numNames > 1)
                    {
                        state = autoSprintf("In The %s Family", lastName);
                    }
                    else
                    {
                        // TODO: when does this happend, other instances except NONAME...
                        state = stringDuplicate(ourName);
                    }
                }
                else if (0 != strcmp(ourName, "NAMELESS"))
                {
                    state = autoSprintf("As %s", ourName);
                }
                else
                {
                    state = stringDuplicate("NAMELESS");
                }
                updateActivity(ActivityType::DEATH_SCREEN, details, state);
                delete[] details;
                delete[] state;
                delete[] ourName;
            }
            else
            {
                updateActivity(ActivityType::DEATH_SCREEN, "Died", "");
            }
        }
        dLastWasIdle = isIdle;
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
        char lastActivityMainMenu = ActivityType::IN_MAIN_MENU == getCurrentActivity();
        char isIdle = lastActivityMainMenu && time(0) - lastStateSetAt > idleTime;
        if (dLastWasIdle != isIdle || !dFirstReportDone || dDisplayStatus != dLastDisplayStatus || !lastActivityMainMenu)
        {
            if (!lastActivityMainMenu)
            {
                lastStateSetAt = time(0);
            }
            char *details = autoSprintf("In Main Menu%s", (isIdle ? " [IDLE]" : ""));
            updateActivity(ActivityType::IN_MAIN_MENU, details, "");
            delete[] details;
        }
        dLastWasIdle = isIdle;
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
            printf("discord error lazyUpdateRichPresence(): dataPage is NULL, will early return.\n");
            return;
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
        connect();
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
void DiscordController::updateDisplayAge(char newValue)
{
    printf("discord: DisplayAge was changed to %d\n", newValue);
    dDisplayAge = newValue;
}
