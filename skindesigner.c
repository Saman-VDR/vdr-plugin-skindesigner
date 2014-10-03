/*
 * skindesigner.c: A plugin for the Video Disk Recorder
 *
 * See the README file for copyright information and how to reach the author.
 *
 * $Id$
 */
#include <getopt.h>
#include <vdr/plugin.h>

#define DEFINE_CONFIG 1
#include "config.h"
#include "designer.h"
#include "setup.h"


#if defined(APIVERSNUM) && APIVERSNUM < 20000 
#error "VDR-2.0.0 API version or greater is required!"
#endif


static const char *VERSION        = "0.0.2";
static const char *DESCRIPTION    = "SkinDesigner";
static const char *MAINMENUENTRY  = "Skin Designer";

class cPluginSkinDesigner : public cPlugin {
private:
    vector<cSkinDesigner*> skins;
public:
    cPluginSkinDesigner(void);
    virtual ~cPluginSkinDesigner();
    virtual const char *Version(void) { return VERSION; }
    virtual const char *Description(void) { return DESCRIPTION; }
    virtual const char *CommandLineHelp(void);
    virtual bool ProcessArgs(int argc, char *argv[]);
    virtual bool Initialize(void);
    virtual bool Start(void);
    virtual void Stop(void);
    virtual void Housekeeping(void);
    virtual void MainThreadHook(void);
    virtual cString Active(void);
    virtual time_t WakeupTime(void);
    virtual const char *MainMenuEntry(void) {return NULL;}
    virtual cOsdObject *MainMenuAction(void);
    virtual cMenuSetupPage *SetupMenu(void);
    virtual bool SetupParse(const char *Name, const char *Value);
    virtual bool Service(const char *Id, void *Data = NULL);
    virtual const char **SVDRPHelpPages(void);
    virtual cString SVDRPCommand(const char *Command, const char *Option, int &ReplyCode);
};

cPluginSkinDesigner::cPluginSkinDesigner(void) {
}

cPluginSkinDesigner::~cPluginSkinDesigner() {
}

const char *cPluginSkinDesigner::CommandLineHelp(void) {
  return
         "  -s <SKINPATH>, --skinpath=<SKINPATH> Set directory where xml skins are stored\n"
         "  -l <LOGOPATH>, --logopath=<LOGOPATH> Set directory where a common logo set for all skins is stored\n"
         "  -e <EPGIMAGESPATH>, --epgimages=<IMAGESPATH> Set directory where epgimages are stored\n";
}

bool cPluginSkinDesigner::ProcessArgs(int argc, char *argv[]) {
  // Implement command line argument processing here if applicable.
    static const struct option long_options[] = {
        { "epgimages", required_argument, NULL, 'e' },
        { "logopath", required_argument, NULL, 'l' },
        { "skinpath", required_argument, NULL, 's' },
        { 0, 0, 0, 0 }
    };

    int c;
    while ((c = getopt_long(argc, argv, "e:s:l:", long_options, NULL)) != -1) {
        switch (c) {
            case 'e':
                config.SetEpgImagePath(cString(optarg));
                break;
            case 'l':
                config.SetLogoPath(cString(optarg));
                break;
            case 's':
                config.SetSkinPath(cString(optarg));
                break;
            default:
                return false;
        }
    }
    return true;
}

bool cPluginSkinDesigner::Initialize(void) {
    return true;
}

bool cPluginSkinDesigner::Start(void) {
    bool trueColorAvailable = true;
    if (!cOsdProvider::SupportsTrueColor()) {
        esyslog("skindesigner: No TrueColor OSD found! Using default Skin LCARS!");
        trueColorAvailable = false;
    } else
        dsyslog("skindesigner: TrueColor OSD found");
    config.SetPathes();
    config.ReadSkins();
    config.InitSkinIterator();
    string skin = "";
    while (config.GetSkin(skin)) {
        cSkinDesigner *newSkin = new cSkinDesigner(skin);
        skins.push_back(newSkin);
        if (!trueColorAvailable) {
            newSkin->ActivateBackupSkin();
        }
    }
    return true;
}

void cPluginSkinDesigner::Stop(void) {
    delete imgCache;
    delete fontManager;
}

void cPluginSkinDesigner::Housekeeping(void) {
}

void cPluginSkinDesigner::MainThreadHook(void) {
}

cString cPluginSkinDesigner::Active(void) {
    return NULL;
}

time_t cPluginSkinDesigner::WakeupTime(void) {
    return 0;
}

cOsdObject *cPluginSkinDesigner::MainMenuAction(void) {
    return NULL;
}

cMenuSetupPage *cPluginSkinDesigner::SetupMenu(void) {
    return new cSkinDesignerSetup();
}

bool cPluginSkinDesigner::SetupParse(const char *Name, const char *Value) {
    return config.SetupParse(Name, Value);
}

bool cPluginSkinDesigner::Service(const char *Id, void *Data) {
    return false;
}

const char **cPluginSkinDesigner::SVDRPHelpPages(void) {
    static const char *HelpPages[] = {
        "RELD\n"
        "    force reload of templates and caches",
        "LSTF\n"
        "    List available Fonts",
        0
    };
    return HelpPages;
}

cString cPluginSkinDesigner::SVDRPCommand(const char *Command, const char *Option, int &ReplyCode) {
    
    cSkinDesigner *activeSkin = NULL;
    for (vector<cSkinDesigner*>::iterator skin = skins.begin(); skin != skins.end(); skin++) {
        string activeSkinName = Setup.OSDSkin;
        string currentSkinName = (*skin)->Description();
        if (!currentSkinName.compare(activeSkinName)) {
            activeSkin = *skin;
            break;
        }
    }

    if (!activeSkin)
        return NULL;

    if (strcasecmp(Command, "RELD") == 0) {
        activeSkin->Reload();
        return "SKINDESIGNER reload of templates and caches forced.";
    } else if (strcasecmp(Command, "LSTF") == 0) {
        activeSkin->ListAvailableFonts();
        return "SKINDESIGNER available fonts listed in syslog.";
    }
    return NULL;
}

VDRPLUGINCREATOR(cPluginSkinDesigner); // Don't touch this!
