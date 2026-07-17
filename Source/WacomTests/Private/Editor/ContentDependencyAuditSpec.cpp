// Copyright Wacom. All Rights Reserved.

#include "Misc/AutomationTest.h"

#if WITH_AUTOMATION_TESTS

#include "../../../WacomEditor/Private/ContentAudit/WacomContentDependencyAudit.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomEditorContentDependencyAuditPathContractSpec,
	"Wacom.Editor.ContentDependencyAudit.PathContract",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomEditorContentDependencyAuditPathContractSpec::RunTest(const FString& Parameters)
{
	using namespace Wacom::ContentAudit;
	const FName WacomRoot(TEXT("/Game/Wacom"));
	TestTrue(TEXT("Root package belongs to itself"), IsPackageUnderRoot(WacomRoot, WacomRoot));
	TestTrue(TEXT("Real subpackage belongs to the root"),
		IsPackageUnderRoot(FName(TEXT("/Game/Wacom/UI/Card")), WacomRoot));
	TestFalse(TEXT("A textual prefix is not treated as a child package"),
		IsPackageUnderRoot(FName(TEXT("/Game/Wacomish/Card")), WacomRoot));
	TestEqual(TEXT("Art dependency classification"),
		ClassifyExternalPackage(FName(TEXT("/Game/Art/Card/T_Card")), WacomRoot), FString(TEXT("Art")));
	TestEqual(TEXT("Generic Asset dependency classification"),
		ClassifyExternalPackage(FName(TEXT("/Game/Asset/Texture/T_Noise")), WacomRoot), FString(TEXT("Asset")));
	TestEqual(TEXT("DreamShader output classification"),
		ClassifyExternalPackage(FName(TEXT("/Game/DreamMaterials/Card/M_Test")), WacomRoot), FString(TEXT("DreamMaterials")));
	TestEqual(TEXT("Legacy test map classification"),
		ClassifyExternalPackage(FName(TEXT("/Game/L_TestBattle")), WacomRoot), FString(TEXT("L_TestBattle")));
	TestEqual(TEXT("Unknown game package remains visible"),
		ClassifyExternalPackage(FName(TEXT("/Game/ThirdParty/Foo")), WacomRoot), FString(TEXT("OtherGame")));

	FWacomContentDependencyAuditReport Report;
	Report.ScanRoot = WacomRoot;
	Report.ScannedPackageCount = 2;
	Report.TraversedGamePackageCount = 3;
	Report.PlaceholderPackages = {
		FName(TEXT("/Game/Wacom/Art/Placeholders/Z_Proxy")),
		FName(TEXT("/Game/Wacom/Art/Placeholders/A_Proxy")) };
	FWacomExternalDependencyFinding& Finding = Report.ExternalFindings.AddDefaulted_GetRef();
	Finding.PackageName = FName(TEXT("/Game/Asset/Texture/T_Noise"));
	Finding.Classification = TEXT("Asset");
	Finding.DirectWacomReferencers = {
		FName(TEXT("/Game/Wacom/UI/Z_Referencer")),
		FName(TEXT("/Game/Wacom/UI/A_Referencer")) };
	Finding.Referencers = Finding.DirectWacomReferencers;
	Finding.ShortestChain = {
		FName(TEXT("/Game/Wacom/UI/A_Referencer")),
		Finding.PackageName };
	Finding.AssetCount = 1;
	Finding.bHasOnDiskAsset = true;
	Finding.AssetClasses = { TEXT("/Script/Engine.Texture2D") };
	Finding.bHasHardReference = true;
	Finding.bHasGameReference = true;
	const FString Json = SerializeReportToJson(Report);
	TestTrue(TEXT("Report contains its stable schema"), Json.Contains(TEXT("\"schemaVersion\": 2")));
	TestTrue(TEXT("Report contains the external package"), Json.Contains(Finding.PackageName.ToString()));
	TestTrue(TEXT("Report records on-disk asset state"), Json.Contains(TEXT("\"hasOnDiskAsset\": true")));
	TestTrue(TEXT("Report records asset classes"), Json.Contains(TEXT("/Script/Engine.Texture2D")));
	TestTrue(TEXT("Referencers are serialized in lexical order"),
		Json.Find(TEXT("A_Referencer")) < Json.Find(TEXT("Z_Referencer")));
	TestTrue(TEXT("Placeholder packages trigger the release gate"),
		HasPlaceholderPackages(Report));
	TestTrue(TEXT("Placeholder packages are serialized in lexical order"),
		Json.Find(TEXT("A_Proxy")) < Json.Find(TEXT("Z_Proxy")));
	TestFalse(TEXT("Report omits volatile timestamps"), Json.Contains(TEXT("generatedAt")));
	return true;
}

#endif
