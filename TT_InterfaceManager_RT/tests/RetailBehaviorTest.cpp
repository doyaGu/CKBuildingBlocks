#include "CKAll.h"
#include "VxWindowFunctions.h"
#include "../NemoArray.h"
#include "../RegistryUtils.h"
#include "../InterfaceManager.h"

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <filesystem>
#include <random>
#include <stdexcept>
#include <string>
#include <system_error>

extern void SetProcessorSpecific_FunctionsPtr();
CKERROR CreateReadRegistryProto(CKBehaviorPrototype **pproto);
int ReadRegistry(const CKBehaviorContext &behcontext);
CKERROR CreateCreateNewGameInfoProto(CKBehaviorPrototype **pproto);
int CreateNewGameInfo(const CKBehaviorContext &behcontext);

static void Check(bool value, const char *message) {
    if (!value) throw std::runtime_error(message);
}

static CKSTRING Str(const char *text) { return const_cast<CKSTRING>(text); }

static void GameInfoUsesTheApplicationRegistryPath() {
    CGameInfo defaults;
    Check(strcmp(defaults.regSubkey, "\"Software\\\\Virtools\\\\Settings\"") == 0,
          "default registry namespace");
    CGameInfo application("Software\\Sample\\Settings");
    Check(strcmp(application.regSubkey, "Software\\Sample\\Settings") == 0,
          "application registry path");
    const std::string oversized(600, 'x');
    CGameInfo bounded(oversized.c_str());
    Check(strlen(bounded.regSubkey) == sizeof(bounded.regSubkey) - 1,
          "registry path is terminated within its storage");
}

// Points the file-backed registry at a scratch directory for the test's lifetime.
class ScratchRegistry {
public:
    ScratchRegistry() {
        const auto temp = std::filesystem::temp_directory_path();
        std::random_device random;
        for (unsigned attempt = 0; attempt < 100; ++attempt) {
            const auto directory = temp / ("tt_im_registry_" + std::to_string(random()) + "_" + std::to_string(attempt));
            std::error_code error;
            // Reserve the directory atomically when test processes run in parallel.
            if (std::filesystem::create_directory(directory, error)) {
                m_Home = directory.string();
                break;
            }
            Check(!error || error == std::errc::file_exists, "create scratch registry dir");
        }
        Check(!m_Home.empty(), "reserve scratch registry dir");
        m_HadHome = VxGetEnvironmentVariable("NEMO_USER_CONFIG_HOME", m_PreviousHome) != FALSE;
        if (!VxSetEnvironmentVariable("NEMO_USER_CONFIG_HOME", m_Home.c_str())) {
            std::error_code error;
            std::filesystem::remove(m_Home, error);
            Check(false, "set config home");
        }
    }
    ~ScratchRegistry() {
        VxSetEnvironmentVariable("NEMO_USER_CONFIG_HOME", m_HadHome ? m_PreviousHome.CStr() : nullptr);
        std::error_code error;
        const auto directory = std::filesystem::path(m_Home) / "NeMo2";
        std::filesystem::remove(directory / "registry.ini", error);
        std::filesystem::remove(directory, error);
        std::filesystem::remove(m_Home, error);
    }

    ScratchRegistry(const ScratchRegistry &) = delete;
    ScratchRegistry &operator=(const ScratchRegistry &) = delete;

private:
    std::string m_Home;
    XString m_PreviousHome;
    bool m_HadHome = false;
};

static void WriteValues(VxConfigRoot root, const char *section, int intValue, float floatValue, const char *str) {
    VxConfig config(root, "");
    config.OpenSection(section, VxConfig::WRITE);
    config.WriteIntegerEntry("Int", intValue);
    config.WriteFloatEntry("Float", floatValue);
    config.WriteStringEntry("Str", str);
}

static void QueryHelpersMirrorRegQueryValueEx() {
    WriteValues(VXCONFIG_ROOT_LOCAL_MACHINE, "Software\\TT\\Query", 7, 1.5f, "abc");

    CKDWORD dword = 0;
    Check(TTQueryRegistryDword(VXCONFIG_ROOT_LOCAL_MACHINE, "Software\\TT\\Query", "Int", &dword) && dword == 7,
          "integer entry reads as its DWORD value");
    Check(TTQueryRegistryDword(VXCONFIG_ROOT_LOCAL_MACHINE, "Software\\TT\\Query", "Float", &dword) && dword == 0x3fc00000,
          "float entry reads as its IEEE bits, like a REG_DWORD written from a float");
    Check(!TTQueryRegistryDword(VXCONFIG_ROOT_LOCAL_MACHINE, "Software\\TT\\Query", "Missing", &dword),
          "missing DWORD fails");

    char exact[4];
    Check(TTQueryRegistryString(VXCONFIG_ROOT_LOCAL_MACHINE, "Software\\TT\\Query", "Str", exact, sizeof(exact)) &&
              std::strcmp(exact, "abc") == 0,
          "string that fits with its terminator is read");
    char small[3];
    Check(!TTQueryRegistryString(VXCONFIG_ROOT_LOCAL_MACHINE, "Software\\TT\\Query", "Str", small, sizeof(small)),
          "string that does not fit fails instead of truncating");
}

class ReadRegistryFixture {
public:
    CKContext *context = nullptr;
    CKBehavior *behavior = nullptr;
    CKBehaviorPrototype *prototype = nullptr;
    CKBehaviorContext call;
    CKParameterLocal *inputs[4] = {};

    ReadRegistryFixture(CKERROR (*createPrototype)(CKBehaviorPrototype **) = CreateReadRegistryProto,
                        int inputCount = 4) {
        SetProcessorSpecific_FunctionsPtr();
        Check(CKStartUp() == CK_OK, "start runtime");
        CKBuildClassHierarchyTable();
        Check(CKCreateContext(&context, nullptr, 0, 0) == CK_OK, "create context");
        behavior = CKBehavior::Cast(context->CreateObject(CKCID_BEHAVIOR, Str("ReadRegistry")));
        Check(behavior != nullptr, "create behavior");
        Check(createPrototype(&prototype) == CK_OK, "create prototype");
        Check(behavior->InitFromPrototype(prototype) == CK_OK, "initialize behavior");
        const CKGUID guids[4] = {CKPGUID_STRING, CKPGUID_STRING, CKPGUID_DATAARRAY, CKPGUID_STRING};
        for (int i = 0; i < inputCount; ++i) {
            inputs[i] = context->CreateCKParameterLocal(Str("in"), guids[i]);
            Check(inputs[i] && behavior->GetInputParameter(i)->SetDirectSource(inputs[i]) == CK_OK, "bind input");
        }
        call.Behavior = behavior;
        call.Context = context;
        call.MessageManager = context->GetMessageManager();
    }
    ~ReadRegistryFixture() {
        if (context) CKCloseContext(context);
        delete prototype;
        CKShutdown();
    }

    void SetString(int input, const char *value) {
        Check(inputs[input]->SetValue(value, (int)std::strlen(value) + 1) == CK_OK, "set string input");
    }

    void Run() {
        behavior->ActivateOutput(0, FALSE);
        behavior->ActivateOutput(1, FALSE);
        ReadRegistry(call);
    }
};

static void NewGameInfoUsesHostDefaultsAndCompositionOverrides() {
    ReadRegistryFixture fx(CreateCreateNewGameInfoProto, 1);
    InterfaceManager *manager = new InterfaceManager(fx.context);
    Check(manager->GetDefaultRegistryPath() == nullptr, "neutral manager has no application override");
    Check(fx.behavior->GetInputParameter(0)->SetDirectSource(nullptr) == CK_OK, "unbind registry path");

    const auto create = [&]() {
        CreateNewGameInfo(fx.call);
        Check(manager->GetGameInfo() != nullptr, "created game info");
        return manager->GetGameInfo();
    };
    const auto clear = [&]() {
        delete manager->GetGameInfo();
        manager->SetGameInfo(nullptr);
    };

    CGameInfo *info = create();
    Check(strcmp(info->regSubkey, "\"Software\\\\Virtools\\\\Settings\"") == 0,
          "unconfigured behavior uses neutral registry path");
    clear();

    manager->SetDefaultRegistryPath("Software\\Sample\\Settings");
    info = create();
    Check(strcmp(info->regSubkey, "Software\\Sample\\Settings") == 0,
          "behavior uses the host registry path after recreation");
    Check(create() == info, "existing game info is retained");
    clear();

    Check(fx.behavior->GetInputParameter(0)->SetDirectSource(fx.inputs[0]) == CK_OK, "bind registry path");
    fx.SetString(0, "Software\\Composition\\Settings");
    info = create();
    Check(strcmp(info->regSubkey, "Software\\Composition\\Settings") == 0,
          "composition registry path overrides the host default");
    clear();
}

static void SingleValueReadsLocalMachine() {
    WriteValues(VXCONFIG_ROOT_LOCAL_MACHINE, "Software\\TT\\Single", 7, 1.5f, "machine");
    WriteValues(VXCONFIG_ROOT_CURRENT_USER, "Software\\TT\\Single", 99, 9.0f, "user");

    ReadRegistryFixture fx;
    fx.SetString(0, "Software\\TT\\Single");
    CKParameterOut *out = fx.behavior->GetOutputParameter(0);

    fx.SetString(1, "Int");
    fx.Run();
    int intValue = 0;
    fx.behavior->GetOutputParameterValue(0, &intValue);
    Check(intValue == 7, "int output comes from HKEY_LOCAL_MACHINE");
    Check(fx.behavior->IsOutputActive(0) && !fx.behavior->IsOutputActive(1), "successful read activates OK only");

    out->SetGUID(CKPGUID_FLOAT);
    fx.SetString(1, "Float");
    fx.Run();
    float floatValue = 0.0f;
    fx.behavior->GetOutputParameterValue(0, &floatValue);
    Check(floatValue == 1.5f, "float output takes the DWORD bits");

    out->SetGUID(CKPGUID_STRING);
    fx.SetString(1, "Str");
    fx.Run();
    Check(out->GetDataSize() == 256, "string output is set with the 256-byte query buffer");
    Check(std::strcmp((const char *)out->GetReadDataPtr(), "machine") == 0, "string output from HKEY_LOCAL_MACHINE");

    out->SetGUID(CKPGUID_INT);
    int previous = 1234;
    out->SetValue(&previous, sizeof(previous));
    fx.SetString(1, "Missing");
    fx.Run();
    fx.behavior->GetOutputParameterValue(0, &intValue);
    Check(fx.behavior->IsOutputActive(0) && fx.behavior->IsOutputActive(1), "failed read activates FAILED and still OK");
    Check(intValue == 1234, "failed read leaves the output value");
}

static void ArrayModeReadsColumnsWithoutOk() {
    {
        VxConfig config(VXCONFIG_ROOT_LOCAL_MACHINE, "");
        config.OpenSection("Software\\TT\\Arrays\\Load\\0\\", VxConfig::WRITE);
        config.WriteIntegerEntry("0", 11);
        config.WriteIntegerEntry("1", 12);
        config.OpenSection("Software\\TT\\Arrays\\Load\\1\\", VxConfig::WRITE);
        config.WriteFloatEntry("0", 2.5f);
        config.OpenSection("Software\\TT\\Arrays\\Load\\2\\", VxConfig::WRITE);
        config.WriteStringEntry("0", "first");
        config.WriteStringEntry("1", "second");
    }

    ReadRegistryFixture fx;
    CKDataArray *array = (CKDataArray *)fx.context->CreateObject(CKCID_DATAARRAY, Str("NotTheKey"));
    Check(array != nullptr, "create array");
    array->InsertColumn(-1, CKARRAYTYPE_INT, Str("i"));
    array->InsertColumn(-1, CKARRAYTYPE_FLOAT, Str("f"));
    array->InsertColumn(-1, CKARRAYTYPE_STRING, Str("s"));
    array->InsertRow();
    array->InsertRow();
    float stale = 9.0f;
    array->SetElementValue(1, 1, &stale, sizeof(stale));

    CKBOOL arrayMode = TRUE;
    fx.behavior->SetLocalParameterValue(0, &arrayMode);
    fx.SetString(0, "Software\\TT\\Arrays");
    CK_ID id = array->GetID();
    fx.inputs[2]->SetValue(&id, sizeof(id));
    fx.SetString(3, "Load");
    fx.Run();

    int i0 = 0, i1 = 0;
    float f0 = 0.0f, f1 = -1.0f;
    char s0[16] = {}, s1[16] = {};
    array->GetElementValue(0, 0, &i0);
    array->GetElementValue(1, 0, &i1);
    array->GetElementValue(0, 1, &f0);
    array->GetElementValue(1, 1, &f1);
    array->GetElementStringValue(0, 2, s0, sizeof(s0));
    array->GetElementStringValue(1, 2, s1, sizeof(s1));
    Check(i0 == 11 && i1 == 12, "int column read per row");
    Check(f0 == 2.5f, "float column read per row");
    Check(f1 == 0.0f, "failed cell is cleared and the loop continues");
    Check(std::strcmp(s0, "first") == 0 && std::strcmp(s1, "second") == 0, "string column read after a failure");
    Check(!fx.behavior->IsOutputActive(0), "array mode never activates OK");
    Check(fx.behavior->IsOutputActive(1), "failed cell activates FAILED");
}

static void WriteKeepsColumnOnTypeMismatch() {
    ReadRegistryFixture fx;
    CKDataArray *source = (CKDataArray *)fx.context->CreateObject(CKCID_DATAARRAY, Str("Saved"));
    source->InsertColumn(-1, CKARRAYTYPE_FLOAT, Str("f"));
    source->InsertColumn(-1, CKARRAYTYPE_INT, Str("i"));
    source->InsertRow();
    float f = 1.5f;
    int i = 7;
    source->SetElementValue(0, 0, &f, sizeof(f));
    source->SetElementValue(0, 1, &i, sizeof(i));
    CNemoArray nemo("level.cmo", source);

    CKDataArray *swapped = (CKDataArray *)fx.context->CreateObject(CKCID_DATAARRAY, Str("Saved"));
    swapped->InsertColumn(-1, CKARRAYTYPE_INT, Str("i"));
    swapped->InsertColumn(-1, CKARRAYTYPE_FLOAT, Str("f"));
    swapped->InsertRow();
    int written = 42;
    float ignored = 8.0f;
    swapped->SetElementValue(0, 0, &written, sizeof(written));
    swapped->SetElementValue(0, 1, &ignored, sizeof(ignored));
    // Element 0 (float) mismatches column 0 (int) and keeps the column; element 1 (int)
    // then takes column 0.
    nemo.Write(swapped);

    float zero = 0.0f;
    int none = 0;
    source->SetElementValue(0, 0, &zero, sizeof(zero));
    source->SetElementValue(0, 1, &none, sizeof(none));
    nemo.Read(source);
    float outF = 0.0f;
    int outI = 0;
    source->GetElementValue(0, 0, &outF);
    source->GetElementValue(0, 1, &outI);
    Check(outF == 1.5f, "mismatched element keeps its value");
    Check(outI == 42, "next element is written from the kept column");
}

int main() {
    try {
        ScratchRegistry registry;
        GameInfoUsesTheApplicationRegistryPath();
        NewGameInfoUsesHostDefaultsAndCompositionOverrides();
        QueryHelpersMirrorRegQueryValueEx();
        SingleValueReadsLocalMachine();
        ArrayModeReadsColumnsWithoutOk();
        WriteKeepsColumnOnTypeMismatch();
        std::puts("TT_InterfaceManager retail behavior tests passed");
        return 0;
    } catch (const std::exception &error) {
        std::fprintf(stderr, "%s\n", error.what());
        return 1;
    }
}
