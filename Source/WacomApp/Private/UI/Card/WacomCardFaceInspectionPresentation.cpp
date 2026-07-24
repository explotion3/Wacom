// Copyright Wacom. All Rights Reserved.

#include "UI/Card/WacomCardFaceInspectionPresentation.h"

namespace WacomCardFaceInspectionPresentation
{
	void AppendToggleHint(FWacomCardDetailViewData& DetailViewData)
	{
		FWacomCardDetailRun HintRun;
		HintRun.StableId = TEXT("FaceInspectToggleHint.Text");
		HintRun.Kind = EWacomCardDetailRunKind::Text;
		HintRun.Text = NSLOCTEXT(
			"WacomCardFaceInspection",
			"ToggleHint",
			"单击卡牌 / Tab / RB 查看另一面");

		FWacomCardDetailBlock HintBlock;
		HintBlock.BlockId = TEXT("FaceInspectToggleHint.Block");
		HintBlock.Kind = EWacomCardDetailBlockKind::Paragraph;
		HintBlock.Runs.Add(MoveTemp(HintRun));

		FWacomCardDetailSection HintSection;
		HintSection.SectionId = TEXT("FaceInspectToggleHint");
		HintSection.Kind = EWacomCardDetailSectionKind::Description;
		HintSection.Blocks.Add(MoveTemp(HintBlock));
		DetailViewData.Sections.Add(MoveTemp(HintSection));
	}
}
