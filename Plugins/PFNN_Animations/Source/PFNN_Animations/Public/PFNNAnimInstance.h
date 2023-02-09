// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimInstance.h"
#include "PFNNAnimInstance.generated.h"

/**
 * 
 */
UCLASS(transient, Blueprintable, BlueprintType)
class PFNN_ANIMATIONS_API UPFNNAnimInstance : public UAnimInstance
{
	GENERATED_BODY()
	
public:

	virtual void NativeInitializeAnimation() override;

	virtual void NativeUpdateAnimation(float arg_DeltaTimeX) override;

	class UTrajectoryComponent* GetOwningTrajectoryComponent();

private:

	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (AllowPrivateAccess = "True"))
	class UTrajectoryComponent* OwningTrajectoryComponent;
};
