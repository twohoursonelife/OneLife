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

#if 0
// print to stdout then flush it.
#define VERBOSE(fmt, ...)                                \
    printf("[VERBOSE] " fmt __VA_OPT__(, ) __VA_ARGS__); \
    fflush(stdout)
#else
// Verbose is currently disabled enable it by replacing "0" with "1" in the #if directive.
#define VERBOSE(fmt, ...)
#endif

extern char userReconnect;

char dDisplayDetails = true; // overriden from setttings
char dDisplayGame = true;    // overriden from setttings

// last time we tried to connect to the discord app
time_t lastReconnectAttempt = 0;
time_t inited_at = 0;

// last reported name to the discord app
char *dLastReportedName = NULL;
// last reported age to the discord app
int dLastReportedAge = -1;
// last reported fertility status to the discord app
char dLastReportedFertility = false;
// last reported AFK status to the discord app
char dLastReportedAfk = false;
// did we previously report details to the discord app?
char dLastReportDetails = false;
// did we report anything previously?
char dFirstReportDone = false;

char invalidKey = false;

// value is from server settings afkEmotionIndex.ini, used to report AFK status
static int afkEmotionIndex = 21;

void DISCORD_CALLBACK OnActivityUpdate(void *data, EDiscordResult result)
{
    VERBOSE("OnActivityUpdate() %d was returned\n", result);
    if (result != EDiscordResult::DiscordResult_Ok)
    {
        printf("discord error OnActivityUpdate(): activity update failed result %d was returned\n", (int)result);
    }
}

DiscordController::DiscordController()
{
    memset(&app, 0, sizeof(app));
    dDisplayGame = SettingsManager::getIntSetting("discordRichPresence", 0) > 0;
    dDisplayDetails = SettingsManager::getIntSetting("discordRichPresenceDetails", 0) > 0;
    printf("discord: discordRichPresence.ini=%d\n", dDisplayGame);
    printf("discord: discordRichPresenceDetails.ini=%d\n", dDisplayDetails);
    dLastReportDetails = dDisplayDetails;
    discordControllerInstance = this;
    inited_at = time(0);
}

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

    // TODO: should we create discord_client_id.ini? (maybe it is not meant to be viewable by public...)
    char *discord_client_id = SettingsManager::getStringSetting("discord_client_id", "1071527161049124914");
    VERBOSE("discord_client_id to parse: %s\n", discord_client_id);
    char *endptr;
    DiscordClientId parsed_client_id = strtoll(discord_client_id, &endptr, 10);
    VERBOSE("discord_client_id after parse: %lld\n", parsed_client_id);
    if (*endptr != '\0')
    {
        // TODO: what happens if discord change the key to accept letters?
        // i guess they have to change the library too in that case because DiscordClientId type is integer
        VERBOSE("discord_client_id key was not fully read from string, key is probably invalid\n");
        invalidKey = true;
        // return EDiscordResult::DiscordResult_InvalidSecret; // let it try one time only
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
    // TODO: it seems it will pass even if id was wrong??, please test
    if (result != EDiscordResult::DiscordResult_Ok)
    {
        printf("discord error connect(): failed to connect to discord client, result returned: %d\n", (int)result);
        memset(&app, 0, sizeof(app));
        return result;
    }

    app.activities = app.core->get_activity_manager(app.core);
    app.application = app.core->get_application_manager(app.core);

    memset(&activity, 0, sizeof(activity));

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
        strncpy(activity.state, state, sizeof(activity.details));
    }
    app.current_activity = activity_type;
    // TODO: try to remove callbacks and see if they are required or not cuz we dont need them.
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
    if (!app.core)
    {
        VERBOSE("discord error runCallbacks(): core not initialized, will skip this call\n");
        return EDiscordResult::DiscordResult_ApplicationMismatch;
    }
    if (!isConnected())
    {
        VERBOSE("discord error runCallbacks(): we are disconnected from discord, will skip this call\n");
        return EDiscordResult::DiscordResult_ApplicationMismatch;
    }
    EDiscordResult result = app.core->run_callbacks(app.core);
    if (result != EDiscordResult::DiscordResult_Ok)
    {
        VERBOSE("discord error runCallbacks(): failed to run callbacks loop, result from run_callbacks(): %d\n", (int)result);
        isHealthy = false;
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

void DiscordController::step(DiscordCurrentGamePage page, GamePage *dataPage)
{
    VERBOSE("DiscordController::step(%d, %p) was called current activity is %d\n", page, dataPage, getCurrentActivity());
    if (!dDisplayGame || !dDisplayDetails)
    {
        VERBOSE("DiscordController::step(%d, %p) early return because dDisplayGame or dDisplayDetails is false\n", page, dataPage);
        return;
    }
    if (!app.core || !isConnected())
    {
        VERBOSE("DiscordController::step(%d, %p) core not initialized or we are not connected, will try connect if (now - lastReconnectAttempt > 10)\n", page, dataPage);
        if (invalidKey)
        {
            VERBOSE("DiscordController::step(%d, %p) invalid key\n", page, dataPage);
            return;
        }
        time_t now = time(0);
        if (now - lastReconnectAttempt > 10)
        {
            lastReconnectAttempt = now;
            printf("discord step(): not connected attempting reconnect now, then will skip this call\n");
            connect(); // TODO: does this block while connecting?
        }
        return;
    }

    if (page == DiscordCurrentGamePage::LIVING_LIFE_PAGE)
    {
        if (dataPage == NULL)
        {
            printf("discord error step(): dataPage is NULL sig fault imminent.\n");
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
            // TODO: not necesarrly that when we have afkEmote means we are really afk!
            char isAfk = ourObject->currentEmot != NULL && getEmotion(afkEmotionIndex) == ourObject->currentEmot;
            char infertileFound, fertileFound;
            char *t1, *t2; // temp swap strings

            t1 = replaceOnce(ourName, "+INFERTILE+", "", &infertileFound);
            delete[] ourName;
            if (!dFirstReportDone || dDisplayDetails != dLastReportDetails || ActivityType::LIVING_LIFE != getCurrentActivity() || dLastReportedName == NULL || 0 != strcmp(dLastReportedName, ourName) || dLastReportedAge != ourAge || dLastReportedFertility != infertileFound || dLastReportedAfk != isAfk)
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
                dLastReportedAge = ourAge;
                dLastReportedFertility = infertileFound;
                dLastReportedAfk = isAfk;
                const char ourGender = getObject(ourObject->displayID)->male ? 'M' : 'F';
                char *details = autoSprintf("Living Life, Age %d [%c]%s%s", ourAge, ourGender, infertileFound ? (char *)" [INF]" : (char *)"", isAfk ? (char *)" [IDLE]" : (char *)"");
                char *state = autoSprintf("%s", ourName);
                updateActivity(ActivityType::LIVING_LIFE, details, state);
                delete[] details;
                delete[] state;
                delete[] ourName;
                dLastReportDetails = dDisplayDetails;
                dFirstReportDone = true;
            }
            else
            {
                delete[] t1;
            }
        }
    }
    else if (page == DiscordCurrentGamePage::DISONNECTED_PAGE)
    {
        if (!dFirstReportDone || dDisplayDetails != dLastReportDetails || ActivityType::DISCONNECTED != getCurrentActivity())
        {
            updateActivity(ActivityType::DISCONNECTED, "DISCONNECTED!", "");
            dLastReportDetails = dDisplayDetails;
            dFirstReportDone = true;
        }
    }
    else if (page == DiscordCurrentGamePage::DEATH_PAGE)
    {
        if (!dFirstReportDone || dDisplayDetails != dLastReportDetails || ActivityType::DEATH_SCREEN != getCurrentActivity())
        {
            if (dataPage == NULL)
            {
                printf("discord error step(): dataPage is NULL sig fault imminent.\n");
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
                dLastReportedAge = ourAge;

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
                char *state = autoSprintf("%s", ourName);
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
            dLastReportDetails = dDisplayDetails;
            dFirstReportDone = true;
        }
    }
    else if (page == DiscordCurrentGamePage::LOADING_PAGE)
    {
        if (!dFirstReportDone || dDisplayDetails != dLastReportDetails || ActivityType::GAME_LOADING != getCurrentActivity())
        {
            updateActivity(ActivityType::GAME_LOADING, "Loading Game...", "");
            dLastReportDetails = dDisplayDetails;
            dFirstReportDone = true;
        }
    }
    else if (page == DiscordCurrentGamePage::WAITING_TO_BE_BORN_PAGE)
    {
        if (!dFirstReportDone || dDisplayDetails != dLastReportDetails || ActivityType::WAITING_TO_BE_BORN != getCurrentActivity())
        {
            if (userReconnect)
                updateActivity(ActivityType::WAITING_TO_BE_BORN, "Reconnecting!...", "");
            else
                updateActivity(ActivityType::WAITING_TO_BE_BORN, "Waiting to be born...", "");
            dLastReportDetails = dDisplayDetails;
            dFirstReportDone = true;
        }
    }
    else if (page == DiscordCurrentGamePage::MAIN_MENU_PAGE)
    {
        if (!dFirstReportDone || dDisplayDetails != dLastReportDetails || ActivityType::IN_MAIN_MENU != getCurrentActivity())
        {
            updateActivity(ActivityType::IN_MAIN_MENU, "In Main Menu", "");
            dLastReportDetails = dDisplayDetails;
            dFirstReportDone = true;
        }
    }
    else if (page == DiscordCurrentGamePage::SETTINGS_PAGE)
    {
        if (!dFirstReportDone || dDisplayDetails != dLastReportDetails || ActivityType::EDITING_SETTINGS != getCurrentActivity())
        {
            updateActivity(ActivityType::EDITING_SETTINGS, "Editing Settings", "");
            dLastReportDetails = dDisplayDetails;
            dFirstReportDone = true;
        }
    }
    else if (page == DiscordCurrentGamePage::LIVING_TUTORIAL_PAGE)
    {
        if (!dFirstReportDone || dDisplayDetails != dLastReportDetails || ActivityType::PLAYING_TUTORIAL != getCurrentActivity())
        {
            if (dataPage == NULL)
            {
                printf("discord error step(): dataPage is NULL sig fault imminent.\n");
                fflush(stdout);
            }
            LivingLifePage *livingLifePage = (LivingLifePage *)dataPage;
            LiveObject *ourObject = livingLifePage->getOurLiveObject();
            char isAfk = false;
            if (ourObject != NULL)
            {
                isAfk = ourObject->currentEmot != NULL && getEmotion(afkEmotionIndex) == ourObject->currentEmot;
            }
            updateActivity(ActivityType::PLAYING_TUTORIAL, "Playing Tutorial", isAfk ? "[IDLE]" : "");
            dLastReportDetails = dDisplayDetails;
            dFirstReportDone = true;
        }
    }
    else if (page == DiscordCurrentGamePage::CONNECTION_LOST_PAGE)
    {
        if (!dFirstReportDone || dDisplayDetails != dLastReportDetails || ActivityType::CONNECTION_LOST != getCurrentActivity())
        {
            updateActivity(ActivityType::CONNECTION_LOST, "DISCONNECTED!", "");
            dLastReportDetails = dDisplayDetails;
            dFirstReportDone = true;
        }
    }
    else
    {
        VERBOSE("discord error step(): unhandled DiscordCurrentGamePage parameter value %d. nothing will be updated\n", page);
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
void DiscordController::updateDisplayDetails(char newValue)
{
    printf("discord: DisplayDetails was changed to %d\n", newValue);
    if (!newValue)
    {
        updateActivity(ActivityType::NO_ACTIVITY, "", "");
        dLastReportDetails = newValue;
    }
    dDisplayDetails = newValue;
}