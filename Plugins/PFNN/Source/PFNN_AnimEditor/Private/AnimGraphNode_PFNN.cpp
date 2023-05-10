

#include "PFNN_AnimEditor/Public/AnimGraphNode_PFNN.h"

#define LOCTEXT_NAMESPACE "PFNNAnimNode"

UAnimGraphNode_PFNN::UAnimGraphNode_PFNN(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer)
{
}

FText UAnimGraphNode_PFNN::GetNodeTitle(ENodeTitleType::Type TitleType) const
{
	return LOCTEXT("PFNNAnimNode", "PFNN");
}

FText UAnimGraphNode_PFNN::GetTooltipText() const
{
	return LOCTEXT("PFNNAnimNode", "PFNN");
}

#undef LOCTEXT_NAMESPACE