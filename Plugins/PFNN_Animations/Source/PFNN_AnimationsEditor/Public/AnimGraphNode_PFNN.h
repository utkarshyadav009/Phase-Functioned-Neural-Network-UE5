// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "AnimNode_PFNN.h"

#include "CoreMinimal.h"
//#include "UObject/ObjectMacros.h"
#include "AnimGraphNode_Base.h"
#include "AnimGraphNode_PFNN.generated.h"

/**
 * 
 */
UCLASS()
class PFNN_ANIMATIONSEDITOR_API UAnimGraphNode_PFNN: public UAnimGraphNode_Base
{
	GENERATED_UCLASS_BODY()
	
	UPROPERTY(EditAnywhere, Category=Settings)
	FAnimNode_PFNN Node;

public:

	// UEdGraphNode interface
	/**
	 * Default AnimGraphNode function to get title
	 * @param[in] TitleType, blueprint title of this object 
	 * @return Tilte of this animation node
	 */
	virtual FText GetNodeTitle(ENodeTitleType::Type arg_TitleType) const override;
	/**
	 * Default AnimGraphNode function to get GetTooltipText
	 * @return GetTooltipText of this animation node
	 */
	virtual FText GetTooltipText() const override;
	// End of UEdGraphNode interface
};
