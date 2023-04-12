

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"

#include "ThirdParty/glm/glm.hpp"
#include "ThirdParty/Eigen/Dense"

#include "TrajectoryComponent.generated.h"


UCLASS(BlueprintType)
class PFNN_API UTrajectoryComponent : public UActorComponent
{
	GENERATED_BODY()

public:	
	UTrajectoryComponent();
	~UTrajectoryComponent();

	glm::vec3 GetRootPosition() const;
	glm::vec3 GetPreviousRootPosition() const;
	glm::mat3 GetRootRotation() const;
	glm::mat3 GetPreviousRootRotation() const;

	/*
	* @Descrition: Default TickComponent function - Used for Drawing Debug Trajectory
	*/
	virtual void TickComponent(float arg_DeltaTime, ELevelTick arg_TickType, FActorComponentTickFunction* arg_ThisTickFunction) override;

	/*
	* @Description: Logs all the important trajectory data into a single file
	*/
	void LogTrajectoryData(int arg_FrameCount);

	/*
	* @Description: Tick function for updating the trajectory - Called from animation node to ensure correct tick speed compared to the network
	*/
	void TickTrajectory();

	/*
	* @Description: Updates the past trajectory by retrieving current and updating each past node
	*/
	void ResetTrajectory(const glm::vec3& arg_RootPosition, const glm::mat3& root_rotation);
	void UpdatePastTrajectory();
	void UpdateCurrentTrajectory(const float StandAmount, Eigen::ArrayXf& PFNN_Yp);
	void UpdateFutureTrajectory(Eigen::ArrayXf& PFNN_Yp);

protected:

	virtual void BeginPlay() override;

private:

	void TickInput();
	void CalculateTargetDirection();
	void TickGaits();
	void PredictFutureTrajectory();
	void TickRotations();
	void TickHeights();

	/*
	* @Description Returnes a liniar bled between the X and Y vector direction by using the floating point scalar
	* @Param[in] X vector
	* @Param[in] Y vector
	* @Param[in] Scalar
	* @Return The result of liniar blending X, Y and the scalar
	*/
	static glm::vec3 MixDirections(const glm::vec3 arg_XDirection, const glm::vec3 arg_YDirection, const float arg_Scalar);

	void DrawDebugTrajectory();

public:

	enum { LENGTH = 120 };

	//LOG THESE VARIABLES

	//Basic values
	glm::vec3 TargetDirection;
	glm::vec3 TargetVelocity;
	UPROPERTY(EditAnywhere, Category = BaseValues)
	float Width;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = BaseValues)
	float StrafeAmount;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = BaseValues)
	float StrafeTarget;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = BaseValues)
	float Responsive;

	int tickcounter;

	//Extra smoothing values
	UPROPERTY(EditAnywhere, Category = ExtraSmoothing)
	float ExtraVelocitySmooth;
	UPROPERTY(EditAnywhere, Category = ExtraSmoothing)
	float ExtraStrafeVelocity;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = ExtraSmoothing)
	float ExtraDirectionSmooth;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = ExtraSmoothing)
	float ExtraStrafeSmooth;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = ExtraSmoothing)
	float ExtraGaitSmooth;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = ExtraSmoothing)
	float ExtraJointSmooth;

	//Positional data
	glm::vec3 Positions[LENGTH];
	glm::vec3 Directions[LENGTH];
	glm::mat3 Rotations[LENGTH];
	UPROPERTY(VisibleAnywhere, Category = PositionalData)
	float Heights[LENGTH];

	//Gaits
	UPROPERTY(VisibleAnywhere, Category = Gaits)
	float GaitStand[LENGTH];
	UPROPERTY(VisibleAnywhere, Category = Gaits)
	float GaitWalk[LENGTH];
	UPROPERTY(VisibleAnywhere, Category = Gaits)
	float GaitJog[LENGTH];
	UPROPERTY(VisibleAnywhere, Category = Gaits)
	float GaitJump[LENGTH];
	UPROPERTY(VisibleAnywhere, Category = Gaits)
	float GaitBump[LENGTH];
	UPROPERTY(VisibleAnywhere, Category = Gaits)
	float GaitCrouch[LENGTH];
private:

	UPROPERTY()
	APawn* OwnerPawn;

	glm::vec2 CurrentFrameInput;

	UPROPERTY(EditAnywhere, Category = PFNNDebugging, meta = (AllowPrivateAccess = "True"))
	bool bIsTrajectoryDebuggingEnabled;

};
