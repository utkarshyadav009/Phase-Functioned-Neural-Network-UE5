

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "PFNNCharacter.generated.h"

UCLASS()
class PFNN_API APFNNCharacter : public ACharacter
{
	GENERATED_BODY()

	/** Camera boom positioning the camera behind the character */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Camera, meta = (AllowPrivateAccess = "true"))
	class USpringArmComponent* CameraBoom;

	/** Follow camera */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Camera, meta = (AllowPrivateAccess = "true"))
	class UCameraComponent* FollowCamera;
	 
public:

	// Sets default values for this character's properties
	APFNNCharacter();

	/** Base turn rate, in deg/sec. Other scaling may affect final turn rate. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Input)
	float TurnRateGamepad;

	class UTrajectoryComponent* GetTrajectoryComponent();

	bool HasDebuggingEnabled();

	/** Returns CameraBoom subobject **/
	FORCEINLINE class USpringArmComponent* GetCameraBoom() const { return CameraBoom; }
	/** Returns FollowCamera subobject **/
	FORCEINLINE class UCameraComponent* GetFollowCamera() const { return FollowCamera; }

	
	UFUNCTION(BlueprintCallable, Category = "PFNN")
		void GetPFNNLoaded(bool& bPFNNLoaded);
	
	UFUNCTION(BlueprintCallable, Category = "PFNN")
		void GetLeftLegJointTransform(FTransform& LeftUpLeg, FTransform& LeftLeg, FTransform& LeftFoot, FTransform& LeftToeBase);

	UFUNCTION(BlueprintCallable, Category = "PFNN")
		void GetRightLegJointTransform(FTransform& RightUpLeg, FTransform& RightLeg, FTransform& RightFoot, FTransform& RightToeBase);

	UFUNCTION(BlueprintCallable, Category = "PFNN")
		void GetLeftArmJointTransform(FTransform& LeftArm, FTransform& LeftForeArm, FTransform& LeftHand);

	UFUNCTION(BlueprintCallable, Category = "PFNN")
		void GetRightArmJointTransform(FTransform& RightArm, FTransform& RightForeArm, FTransform& RightHand);

	UFUNCTION(BlueprintCallable, Category = "PFNN")
		void GetSpineJointTransform(FTransform& Spine, FTransform& Spine1, FTransform& Neck, FTransform& Head);
	
	void SetJointTransformForControlRig(FTransform JointTransform, int index, FName JointName);

	void SetPFNNLoaded(bool bPFNNLoaded);
protected:

	/** Called for forwards/backward input */
	void MoveForward(float Value);

	/** Called for side to side input */
	void MoveRight(float Value);

	/**
	 * Called via input to turn at a given rate.
	 * @param Rate	This is a normalized rate, i.e. 1.0 means 100% of desired turn rate
	 */
	void TurnAtRate(float Rate);

	/**
	 * Called via input to turn look up/down at a given rate.
	 * @param Rate	This is a normalized rate, i.e. 1.0 means 100% of desired turn rate
	 */
	void LookUpAtRate(float Rate);

	/** Handler for when a touch input begins. */
	void TouchStarted(ETouchIndex::Type FingerIndex, FVector Location);

	/** Handler for when a touch input stops. */
	void TouchStopped(ETouchIndex::Type FingerIndex, FVector Location);

	// APawn interface
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;
	// End of APawn interface

	void OnJogPressed();
	void OnJogReleased();

	void OnCrouchPressed();
	void OnCrouchReleased();

private:

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, meta = (AllowPrivateAccess = "True"))
	class UTrajectoryComponent* TrajectoryComponent;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = PFNNDebugging ,meta = (AllowPrivateAccess = "True"))
	bool bIsSkeletonDebuggingEnabled;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = PFNNControlRig, meta = (AllowPrivateAccess = "True"))
	TArray<FTransform> PFNNJointTransformControlRig;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = PFNNControlRig, meta = (AllowPrivateAccess = "True"))
	TArray<FName> JointNameByIndex;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, meta = (AllowPrivateAccess = "True"))
	class UPFNNAnimInstance* PFNNAnimInstance;

	bool bIsPFNNLoaded;
};
