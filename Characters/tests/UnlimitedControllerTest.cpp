#include "CKAll.h"
#include "RCKRenderObject.h"
#include "RCK3dEntity.h"
#include "RCK3dObject.h"
#include "RCKCharacter.h"
#include "RCKAnimation.h"
#include "../Behaviors/UnlimitedParams.h"
#include <cstdio>
#include <cstring>
#include <stdexcept>

extern void SetProcessorSpecific_FunctionsPtr();
CKERROR CreateUnlimitedControllerBehaviorProto(CKBehaviorPrototype **prototype);
int DoUnlimitedController(const CKBehaviorContext &context);
void CheckAnimations(CKContext *context, int count, UnlimitedParam *parameters,
                     const XObjectArray &animations, CKCharacter *owner);

static void Check(bool value, const char *message) {
    if (!value) throw std::runtime_error(message);
}

static void Word(unsigned char *bytes, int offset, unsigned int value) {
    for (int i = 0; i < 4; ++i) bytes[offset + i] = static_cast<unsigned char>(value >> (i * 8));
}

// Historical Win32 record offsets, independent of sizeof(UnlimitedParam).
static void LegacyRecord(unsigned char (&bytes)[152], bool secondary) {
    std::memset(bytes, 0, sizeof(bytes));
    Word(bytes, 0, 0);
    Word(bytes, 4, secondary ? 0x3fa00000 : 7); // StartFrame 1.25 / Priority 7.
    std::memcpy(bytes + 8, "Legacy animation", 17);
    Word(bytes, 48, 0x12345678); // Obsolete runtime pointer; rebuilt on load.
    std::memset(bytes + 56, 0xff, 16);
    std::memcpy(bytes + 72, "Run", 4);
    Word(bytes, 140, 0x41f00000); Word(bytes, 144, 0x40a00000); // 30 fps, warp 5.
    bytes[148] = PARAM_LOOP; bytes[150] = 1;
}

class UnlimitedControllerTestFixture {
public:
    CKContext *context = nullptr;
    CKBehavior *behavior = nullptr;
    CKCharacter *character = nullptr;
    CKAnimation *animation = nullptr;
    CKBehaviorPrototype *prototype = nullptr;
    CKBehaviorContext call;

    UnlimitedControllerTestFixture() {
        SetProcessorSpecific_FunctionsPtr();
        Check(CKStartUp() == CK_OK, "start runtime");
        CKCLASSREGISTERCID(RCKRenderObject, CKCID_BEOBJECT);
        CKCLASSREGISTERCID(RCK3dEntity, CKCID_RENDEROBJECT);
        CKCLASSREGISTERCID(RCK3dObject, CKCID_3DENTITY);
        CKCLASSREGISTERCID(RCKCharacter, CKCID_3DOBJECT);
        CKCLASSREGISTERCID(RCKAnimation, CKCID_SCENEOBJECT);
        CKBuildClassHierarchyTable();
        Check(CKCreateContext(&context, nullptr, 0, 0) == CK_OK, "create context");
        character = CKCharacter::Cast(context->CreateObject(CKCID_CHARACTER, "Character"));
        animation = CKAnimation::Cast(context->CreateObject(CKCID_ANIMATION, "Animation"));
        behavior = CKBehavior::Cast(context->CreateObject(CKCID_BEHAVIOR, "Controller"));
        Check(character && animation && behavior, "create controller objects");
        Check(CreateUnlimitedControllerBehaviorProto(&prototype) == CK_OK, "create prototype");
        Check(behavior->InitFromPrototype(prototype) == CK_OK, "initialize controller");
        behavior->SetVersion(0x20000);
        Check(behavior->SetOwner(character, FALSE) == CK_OK, "set controller owner");
        call.Behavior = behavior; call.Context = context;
        call.MessageManager = context->GetMessageManager();
        call.CallbackMessage = CKM_BEHAVIORLOAD;
        unsigned char primary[152], secondary[152];
        LegacyRecord(primary, false); LegacyRecord(secondary, true);
        int count = 1;
        Check(behavior->SetLocalParameterValue(1, primary, 152) == CK_OK, "set primary record");
        Check(behavior->SetLocalParameterValue(2, &count) == CK_OK, "set primary count");
        Check(behavior->SetLocalParameterValue(6, secondary, 152) == CK_OK, "set secondary record");
        Check(behavior->SetLocalParameterValue(7, &count) == CK_OK, "set secondary count");
        XObjectArray *animations = nullptr;
        Check(behavior->GetLocalParameterValue(0, &animations) == CK_OK && animations, "get animations");
        animations->PushBack(animation->GetID());
    }
    ~UnlimitedControllerTestFixture() {
        if (context) CKCloseContext(context);
        delete prototype;
        CKShutdown();
    }
};

static void ExpectRecord(CKBehavior *behavior, int index, bool secondary, CK_ID animation) {
    Check(behavior->GetLocalParameter(index)->GetDataSize() == 152, "historical record size");
    UnlimitedParam record;
    Check(behavior->GetLocalParameterValue(index, &record) == CK_OK, "read record");
    Check(record.IndexAnimation == 0 && record.AnimationId == animation, "rebuild animation cache");
    Check(secondary ? record.StartFrame == 1.25f : record.Priority == 7, "preserve numeric union");
    Check(record.fps == 30 && record.warplength == 5, "preserve floating values");
    Check(record.flags == PARAM_LOOP && record.LoopCount == 1, "preserve playback flags");
    Check(std::strcmp(record.Message, "Run") == 0, "preserve message");
}

static void LoadLegacyRecordsAndRoundTripParameter() {
    UnlimitedControllerTestFixture fixture;
    CKParameterLocal *data = fixture.behavior->GetLocalParameter(1);
    CKStateChunk *saved = data->Save(nullptr, CK_STATESAVE_ALL);
    Check(saved != nullptr, "save legacy parameter");
    CKParameterLocal *loaded = fixture.context->CreateCKParameterLocal("Reloaded", CKPGUID_VOIDBUF);
    Check(loaded && loaded->Load(saved, nullptr) == CK_OK, "reload legacy parameter");
    DeleteCKStateChunk(saved);
    Check(loaded->GetDataSize() == 152, "parameter round trip keeps record size");
    Check(std::memcmp(data->GetReadDataPtr(FALSE), loaded->GetReadDataPtr(FALSE), 152) == 0,
          "parameter round trip keeps every serialized byte");
    Check(fixture.behavior->SetLocalParameterValue(1, loaded->GetReadDataPtr(FALSE), 152) == CK_OK,
          "use reloaded primary record");
    Check(fixture.behavior->CallCallbackFunction(CKM_BEHAVIORLOAD) == CKBR_OK, "load primary and secondary records");
    ExpectRecord(fixture.behavior, 1, false, fixture.animation->GetID());
    ExpectRecord(fixture.behavior, 6, true, fixture.animation->GetID());
}

static void ReorderingAndDeletionUpdateAnimationCache() {
    UnlimitedControllerTestFixture fixture;
    Check(fixture.behavior->CallCallbackFunction(CKM_BEHAVIORLOAD) == CKBR_OK, "load animation cache");
    UnlimitedParam record;
    Check(fixture.behavior->GetLocalParameterValue(1, &record) == CK_OK, "get cached record");
    CKAnimation *other = CKAnimation::Cast(fixture.context->CreateObject(CKCID_ANIMATION, "Other"));
    Check(other != nullptr, "create second animation");
    XObjectArray order; order.PushBack(other->GetID()); order.PushBack(fixture.animation->GetID());
    CheckAnimations(fixture.context, 1, &record, order, nullptr);
    Check(record.IndexAnimation == 1 && record.AnimationId == fixture.animation->GetID(), "follow reordered animation");
    fixture.context->DestroyObject(fixture.animation);
    order.Check(fixture.context);
    CheckAnimations(fixture.context, 1, &record, order, nullptr);
    Check(record.IndexAnimation == -1 && record.AnimationId == 0, "clear deleted animation at old last index");
}

static void InvalidCountsDoNotChangeRecords() {
    UnlimitedControllerTestFixture fixture;
    unsigned char before[152];
    Check(fixture.behavior->GetLocalParameterValue(1, before) == CK_OK, "read original data");
    for (int index = 2; index <= 7; index += 5) {
        const int invalid[] = {-1, 2};
        for (int count : invalid) {
            Check(fixture.behavior->SetLocalParameterValue(index, &count) == CK_OK, "set invalid count");
            Check(fixture.behavior->CallCallbackFunction(CKM_BEHAVIORLOAD) == CKERR_INVALIDPARAMETER, "reject invalid callback count");
            Check(DoUnlimitedController(fixture.call) == CKBR_GENERICERROR, "reject invalid execution count");
            Check(std::memcmp(before, fixture.behavior->GetLocalParameterReadDataPtr(1), 152) == 0,
                  "invalid count leaves primary bytes unchanged");
        }
        int one = 1; fixture.behavior->SetLocalParameterValue(index, &one);
    }
}

int main() {
    try {
        LoadLegacyRecordsAndRoundTripParameter();
        ReorderingAndDeletionUpdateAnimationCache();
        InvalidCountsDoNotChangeRecords();
        std::puts("Unlimited Controller record, cache, and count tests passed");
        return 0;
    } catch (const std::exception &error) {
        std::fprintf(stderr, "%s\n", error.what());
        return 1;
    }
}
