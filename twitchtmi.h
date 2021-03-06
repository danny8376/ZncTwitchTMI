#pragma once

#include <znc/Modules.h>
#include <znc/Threads.h>

#include <unordered_set>
#include <unordered_map>
#include <functional>
#include <vector>
#include <ctime>
#include <list>

class TwitchTMIUpdateTimer;
class TwitchTokenRefreshTimer;

class TwitchTMI : public CModule
{
    friend class TwitchTMIUpdateTimer;
    friend class TwitchTokenRefreshTimer;
    friend class TwitchTMIJob;

    public:
    MODCONSTRUCTOR(TwitchTMI),lastFrankerZ(0),noLastPlay(false) {}
    virtual ~TwitchTMI();

    bool OnLoad(const CString &sArgsi, CString &sMessage) override;
    bool OnBoot() override;

    CString GetWebMenuTitle() override;
    bool OnWebRequest(CWebSock &sock, const CString &pageName, CTemplate &tmpl) override;

    CModule::EModRet OnIRCRegistration(CString &sPass, CString &sNick, CString &sIdent, CString &sRealName) override;
    void OnIRCConnected() override;

    CModule::EModRet OnUserRawMessage(CMessage &msg) override;
    CModule::EModRet OnRawMessage(CMessage &msg) override;
    CModule::EModRet OnUserJoin(CString &sChannel, CString &sKey) override;
    CModule::EModRet OnPrivTextMessage(CTextMessage &Message) override;
    CModule::EModRet OnChanTextMessage(CTextMessage &Message) override;
    CModule::EModRet OnUserTextMessage(CTextMessage &msg) override;
    bool OnServerCapAvailable(const CString &sCap) override;

    private:
    CString GetTwitchAccessToken();
    void DoTwitchLogin(const CString &code);
    void InjectMessageHelper(CChan *chan, const CString &action);
    void PutUserChanMessage(CChan *chan, const CString &format, const CString &text);

    private:
    TwitchTMIUpdateTimer *updateTimer;
    TwitchTokenRefreshTimer *refreshTimer;
    std::time_t lastFrankerZ;
    std::unordered_map<CString, std::pair<std::time_t, int> > lastPlay;
    bool noLastPlay;
    std::unordered_set<CString> liveChannels;
    std::mutex job_thread_lock;
};

class TwitchTMIUpdateTimer : public CTimer
{
    friend class TwitchTMI;

    public:
    TwitchTMIUpdateTimer(TwitchTMI *mod);

    private:
    void RunJob() override;

    private:
    TwitchTMI *mod;
};

class TwitchTokenRefreshTimer : public CTimer
{
    friend class TwitchTMI;

    public:
    TwitchTokenRefreshTimer(TwitchTMI *mod);

    private:
    void RunJob() override;

    private:
    TwitchTMI *mod;
};

class TwitchTMIJob : public CModuleJob
{
    public:
    TwitchTMIJob(TwitchTMI *mod, const std::list<CString> &channels)
        :CModuleJob(mod, "twitch_updates", "fetches updates from twitch")
        ,mod(mod)
        ,channels(channels) {}

    void runThread() override;
    void runMain() override;

    private:
    TwitchTMI *mod;
    std::list<CString> channels;
    std::unordered_map<CString, CString> titles;
    std::unordered_map<CString, bool> lives;
};

class GenericJob : public CModuleJob
{
    public:
    GenericJob(CModule* mod, const CString& name, const CString& desc, std::function<void()> threadFunc, std::function<void()> mainFunc)
        :CModuleJob(mod, name, desc)
        ,threadFunc(threadFunc),mainFunc(mainFunc) {}

    void runThread() override { threadFunc(); }
    void runMain() override { mainFunc(); }

    private:
    std::function<void()> threadFunc;
    std::function<void()> mainFunc;
};

class GenericTimer : public CTimer
{
    public:
    GenericTimer(CModule* mod, unsigned int interval, unsigned int cycles, const CString& label, const CString& desc, std::function<void()> runFunc)
        :CTimer(mod, interval, cycles, label, desc)
        ,runFunc(runFunc) {}

    protected:
    void RunJob() override { runFunc(); }

    private:
    std::function<void()> runFunc;
};

