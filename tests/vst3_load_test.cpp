// Minimal Windows VST3 load test: mimics what a host does when scanning a plugin.
// Build with llvm-mingw, run under Wine:  vst3_load_test.exe "path\to\Plugin.vst3"
#include <windows.h>
#include <cstdio>
#include <cstring>
#include <vector>

#include "pluginterfaces/base/ipluginbase.h"
#include "pluginterfaces/vst/ivstcomponent.h"
#include "pluginterfaces/vst/ivstaudioprocessor.h"
#include "pluginterfaces/vst/ivsteditcontroller.h"
#include "pluginterfaces/gui/iplugview.h"
#include "pluginterfaces/vst/ivstmessage.h"

// IIDs are defined inline by compiling with -DINIT_CLASS_IID.

using namespace Steinberg;
typedef IPluginFactory* (PLUGIN_API *GetFactoryProc)();
typedef bool (PLUGIN_API *InitDllProc)();

int main (int argc, char** argv)
{
    if (argc < 2) { std::printf ("usage: vst3_load_test <dll>\n"); return 2; }
    SetErrorMode (SEM_FAILCRITICALERRORS);

    std::printf ("[1] LoadLibrary %s\n", argv[1]); std::fflush (stdout);
    HMODULE m = LoadLibraryA (argv[1]);
    if (! m) { std::printf ("    FAILED, GetLastError=%lu\n", GetLastError()); return 1; }

    if (auto init = (InitDllProc) GetProcAddress (m, "InitDll"))
    {
        std::printf ("[2] InitDll\n"); std::fflush (stdout);
        std::printf ("    -> %d\n", (int) init());
    }

    auto getFactory = (GetFactoryProc) GetProcAddress (m, "GetPluginFactory");
    if (! getFactory) { std::printf ("    no GetPluginFactory export\n"); return 1; }
    std::printf ("[3] GetPluginFactory\n"); std::fflush (stdout);
    IPluginFactory* f = getFactory();
    if (! f) { std::printf ("    returned null\n"); return 1; }

    PFactoryInfo fi {};
    f->getFactoryInfo (&fi);
    std::printf ("    vendor=%s\n", fi.vendor);
    const int n = f->countClasses();
    std::printf ("[4] %d classes\n", n);

    for (int i = 0; i < n; ++i)
    {
        PClassInfo ci {};
        f->getClassInfo (i, &ci);
        std::printf ("    class %d: %s [%s]\n", i, ci.name, ci.category); std::fflush (stdout);
        if (std::strcmp (ci.category, kVstAudioEffectClass) != 0) continue;

        Vst::IComponent* comp = nullptr;
        std::printf ("[5] createInstance(IComponent)\n"); std::fflush (stdout);
        if (f->createInstance (ci.cid, Vst::IComponent::iid, (void**) &comp) != kResultOk || ! comp)
        { std::printf ("    FAILED\n"); return 1; }

        std::printf ("[6] initialize\n"); std::fflush (stdout);
        std::printf ("    -> %d\n", (int) comp->initialize (nullptr));

        Vst::IAudioProcessor* proc = nullptr;
        comp->queryInterface (Vst::IAudioProcessor::iid, (void**) &proc);
        if (proc)
        {
            Vst::ProcessSetup setup { Vst::kRealtime, Vst::kSample32, 512, 48000.0 };
            std::printf ("[7] setupProcessing -> %d\n", (int) proc->setupProcessing (setup)); std::fflush (stdout);
            std::printf ("[8] setActive(true) -> %d\n", (int) comp->setActive (true)); std::fflush (stdout);

            // Process one block with a note-free buffer.
            std::vector<float> l (512), r (512);
            float* chans[2] { l.data(), r.data() };
            Vst::AudioBusBuffers out {}; out.numChannels = 2; out.channelBuffers32 = chans;
            Vst::ProcessData pd {}; pd.processMode = Vst::kRealtime; pd.symbolicSampleSize = Vst::kSample32;
            pd.numSamples = 512; pd.numOutputs = 1; pd.outputs = &out;
            proc->setProcessing (true);
            std::printf ("[8b] process -> %d\n", (int) proc->process (pd)); std::fflush (stdout);
            proc->setProcessing (false);
            comp->setActive (false);
            proc->release();
        }

        // Edit controller + editor window (hosts open this when verifying).
        TUID ctrlCid {};
        comp->getControllerClassId (ctrlCid);
        Vst::IEditController* ctrl = nullptr;
        if (f->createInstance (ctrlCid, Vst::IEditController::iid, (void**) &ctrl) == kResultOk && ctrl)
        {
            const int ir = (int) ctrl->initialize (nullptr);
            Vst::IConnectionPoint *cpComp = nullptr, *cpCtrl = nullptr;
            comp->queryInterface (Vst::IConnectionPoint::iid, (void**) &cpComp);
            ctrl->queryInterface (Vst::IConnectionPoint::iid, (void**) &cpCtrl);
            if (cpComp && cpCtrl) { cpComp->connect (cpCtrl); cpCtrl->connect (cpComp); }
            std::printf ("[c1] controller initialize -> %d, connected=%d, params=%d\n", ir, (int) (cpComp && cpCtrl), (int) ctrl->getParameterCount());
            std::fflush (stdout);
            if (auto* view = ctrl->createView (Vst::ViewType::kEditor))
            {
                HWND parent = CreateWindowExA (0, "STATIC", "host", WS_OVERLAPPEDWINDOW, 0, 0, 1600, 600, nullptr, nullptr, GetModuleHandleA (nullptr), nullptr);
                std::printf ("[c2] createView ok, attached -> %d\n", (int) view->attached (parent, kPlatformTypeHWND)); std::fflush (stdout);
                MSG msg; int k = 0;
                for (; k < 50 && PeekMessageA (&msg, nullptr, 0, 0, PM_REMOVE); ++k)
                {
                    std::printf ("      msg 0x%x hwnd=%p\n", msg.message, (void*) msg.hwnd); std::fflush (stdout);
                    DispatchMessageA (&msg);
                }
                std::printf ("[c2b] pumped %d messages\n", k); std::fflush (stdout);
                std::printf ("[c3] removed -> %d\n", (int) view->removed()); std::fflush (stdout);
                view->release();
                DestroyWindow (parent);
            }
            else std::printf ("[c2] createView returned null\n");
            ctrl->terminate();
            ctrl->release();
        }
        std::printf ("[9] terminate -> %d\n", (int) comp->terminate());
        comp->release();
    }
    f->release();
    std::printf ("OK\n");
    return 0;
}
