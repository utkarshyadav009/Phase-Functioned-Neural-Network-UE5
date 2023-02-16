// Fill out your copyright notice in the Description page of Project Settings.


#include "TrajectoryComponent.h"

#include "PFNNCharacter.h"
#include "PFNNHelperFunctions.h"

#include "DrawDebugHelpers.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Engine/World.h"
#include "Engine/Engine.h"
#include "Runtime/NavigationSystem/Public/NavigationSystem.h"

#define GLM_ENABLE_EXPERIMENTAL
#include <ThirdParty/glm/gtc/matrix_transform.hpp>
#include <ThirdParty/glm/gtc/type_ptr.hpp>
#include <ThirdParty/glm/gtx/transform.hpp>
#include <ThirdParty/glm/gtx/quaternion.hpp>

#include <fstream>

// Sets default values for this component's properties
UTrajectoryComponent::UTrajectoryComponent() 
	: ExtraStrafeSmooth(0)
	, ExtraGaitSmooth(0)
	, ExtraJointSmooth(0)
{
	// Set this component to be initialized when the game starts, and to be ticked every frame.  You can turn these features
	// off to improve performance if you don't need them.
	PrimaryComponentTick.bCanEverTick = true;

	// ...
	Width = 25.0f;
	TargetDirection = glm::vec3(0.0f, 0.0f, 1.0f);
	ExtraVelocitySmooth = 0.9f;
	ExtraStrafeVelocity = 0.9f;
	ExtraDirectionSmooth = 0.9f;
	ExtraStrafeSmooth = 0.9f;
	ExtraGaitSmooth = 0.1f;
	ExtraJointSmooth = 0.5f;

	StrafeTarget = 0.0f;
	StrafeAmount = 0.0f;
	Responsive = 0.0f;

	TargetVelocity = glm::vec3();

	Positions[LENGTH] = {glm::vec3(0.0f, 0.0f, 0.0f)};
	Rotations[LENGTH] = {glm::mat4(1.0f)};
	Directions[LENGTH] = {glm::vec3(0.0f, 0.0f, 1.0f)};
	Heights[LENGTH] = {0.0f};
	GaitJog[LENGTH] = {0.0f};
	GaitWalk[LENGTH] = {0.0f};
	GaitBump[LENGTH] = {0.0f};
	GaitJump[LENGTH] = {0.0f};
	GaitStand[LENGTH] = {0.0f};

	CurrentFrameInput = glm::vec2(0.0f);

	OwnerPawn = nullptr;

	bIsTrajectoryDebuggingEnabled = false;
}

UTrajectoryComponent::~UTrajectoryComponent()
{}

// Called when the game starts
void UTrajectoryComponent::BeginPlay()
{
	Super::BeginPlay();

	// ...
	OwnerPawn = Cast<APawn>(GetOwner());
	IsValid(OwnerPawn);

	Directions[LENGTH] = {UPFNNHelperFunctions::XZYTranslationToXYZ(glm::vec3(OwnerPawn->GetActorForwardVector().X, OwnerPawn->GetActorForwardVector().Y, 0.0f))};

	tickcounter = 0;

	GEngine->AddOnScreenDebugMessage(-1, 0.0f, FColor::Yellow, FString::Printf(TEXT("Gait stand")));

}


// Called every frame
void UTrajectoryComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	// ...
	glm::vec3 Direction1 = glm::vec3(1.0f, 0.0, 0.0);
	glm::vec3 Direction2 = glm::vec3(0.0f, 0.0, 0.0f);

	glm::vec3 Mixed = MixDirections(Direction1, Direction2, 0.5f);

#ifdef WITH_EDITOR
	DrawDebugTrajectory();
#endif
}

glm::vec3 UTrajectoryComponent::GetRootPosition() const
{
	const auto loc = GetOwner()->GetActorLocation();
	return UPFNNHelperFunctions::XZYTranslationToXYZ(FVector(loc.X * 0.01,
															 loc.Y * 0.01,
															 Heights[LENGTH / 2]));
}

glm::vec3 UTrajectoryComponent::GetPreviousRootPosition() const
{
	const auto idx = LENGTH / 2 - 1;
	return glm::vec3(
		Positions[idx].x,
		Heights[idx],
		Positions[idx].z);
}

glm::mat3 UTrajectoryComponent::GetRootRotation() const
{
	return Rotations[LENGTH / 2];
}

glm::mat3 UTrajectoryComponent::GetPreviousRootRotation() const
{
	return Rotations[LENGTH / 2 - 1];
}

void UTrajectoryComponent::TickGaits()
{
	// TODO: crouch amount update
	//character->crouched_amount = glm::mix(character->crouched_amount, character->crouched_target, options->extra_crouched_smooth);


	//Updating of the gaits
	const float MovementCutOff = 0.01f;
	const float JogCuttoff = 0.5f;
	const auto TrajectoryLength = glm::length(TargetVelocity);
	//const auto NormalizedTrajectoryLength = glm::normalize(TrajectoryLength);
	const int32 MedianLength = LENGTH / 2;

	const auto updateValue = [ExtraGaitSmooth = ExtraGaitSmooth](float& param, const float value) { param = glm::mix(param, value, ExtraGaitSmooth); };
	if(TrajectoryLength < MovementCutOff)  // stand
	{
		const float StandingClampMin = 0.0f;
		const float StandingClampMax = 1.0f;
		float stand_amount = 1.0f - glm::clamp(TrajectoryLength * 10, StandingClampMin, StandingClampMax);
		updateValue(GaitStand[MedianLength], stand_amount);
		updateValue(GaitWalk[MedianLength], 0.0f);
		updateValue(GaitJog[MedianLength], 0.0f);
		updateValue(GaitCrouch[MedianLength], 0.0f);
		updateValue(GaitJump[MedianLength], 0.0f);
		updateValue(GaitBump[MedianLength], 0.0f);
	}
	else if(const float crouched = 0/*character->crouched_amount*/; crouched > 0.1) // crouched
	{
		updateValue(GaitStand[MedianLength], 0.0f);
		updateValue(GaitWalk[MedianLength], 0.0f);
		updateValue(GaitJog[MedianLength], 0.0f);
		updateValue(GaitCrouch[MedianLength], crouched);
		updateValue(GaitJump[MedianLength], 0.0f);
		updateValue(GaitBump[MedianLength], 0.0f);
	}
	else if(glm::abs(TrajectoryLength) > JogCuttoff) //Jog old
		// else if((SDL_JoystickGetAxis(stick, GAMEPAD_TRIGGER_R) / 32768.0) + 1.0) // jog original
		// else if(glm::abs(TargetVelocity.x) > JogCuttoff || glm::abs(TargetVelocity.y) > JogCuttoff) //Jog old
	{
		updateValue(GaitStand[MedianLength], 0.0f);
		updateValue(GaitWalk[MedianLength], 0.0f);
		updateValue(GaitJog[MedianLength], 1.0f);
		updateValue(GaitCrouch[MedianLength], 0.0f);
		updateValue(GaitJump[MedianLength], 0.0f);
		updateValue(GaitBump[MedianLength], 0.0f);
	}
	else // walk
	{
		updateValue(GaitStand[MedianLength], 0.0f);
		updateValue(GaitWalk[MedianLength], 1.0f);
		updateValue(GaitJog[MedianLength], 0.0f);
		updateValue(GaitCrouch[MedianLength], 0.0f);
		updateValue(GaitJump[MedianLength], 0.0f);
		updateValue(GaitBump[MedianLength], 0.0f);
	}

	if(bIsTrajectoryDebuggingEnabled)
	{
		GEngine->AddOnScreenDebugMessage(-1, 0.0f, FColor::Yellow, FString::Printf(TEXT("%f Gait stand "), GaitStand[LENGTH / 2]));
		GEngine->AddOnScreenDebugMessage(-1, 0.0f, FColor::Yellow, FString::Printf(TEXT("%f Gait walks "), GaitWalk[LENGTH / 2]));
		GEngine->AddOnScreenDebugMessage(-1, 0.0f, FColor::Yellow, FString::Printf(TEXT("%f Gait Jog "), GaitJog[LENGTH / 2]));
		GEngine->AddOnScreenDebugMessage(-1, 0.0f, FColor::Yellow, FString::Printf(TEXT("%f Gait Jump "), GaitJump[LENGTH / 2]));
		GEngine->AddOnScreenDebugMessage(-1, 0.0f, FColor::Yellow, FString::Printf(TEXT("%f Gait Bump "), GaitBump[LENGTH / 2]));
		GEngine->AddOnScreenDebugMessage(-1, 0.0f, FColor::Yellow, FString::Printf(TEXT("%f TrajectoryLength "), TrajectoryLength));
	}
}

void UTrajectoryComponent::PredictFutureTrajectory()
{
	const auto halfLength = LENGTH / 2;
	//Predicting future trajectory
	glm::vec3 TrajectoryPositionsBlend[LENGTH] = {glm::vec3(0.0f)};
	TrajectoryPositionsBlend[halfLength] = Positions[halfLength];

	const float BiasPosition = Responsive ? glm::mix(2.0f, 2.0f, StrafeAmount) : glm::mix(0.5f, 1.0f, StrafeAmount);
	const float BiasDirection = Responsive ? glm::mix(5.0f, 3.0f, StrafeAmount) : glm::mix(2.0f, 0.5f, StrafeAmount);

	for(int i = halfLength + 1; i < LENGTH; i++)
	{
		const float ScalePosition = 1.0f - powf(1.0f - (static_cast<float>(i - halfLength) / halfLength), BiasPosition);
		const float ScaleDirection = 1.0f - powf(1.0f - (static_cast<float>(i - halfLength) / halfLength), BiasDirection);

		const glm::vec3 DistanceFromPreviousNode = Positions[i] - Positions[i - 1];
		TrajectoryPositionsBlend[i] = TrajectoryPositionsBlend[i - 1] + glm::mix(DistanceFromPreviousNode, TargetVelocity, ScalePosition);

		//TODO: Add wall colision for future trajectory - 1519
		/*
		for(int j = 0; j < areas->num_walls(); j++)
		{
			glm::vec2 trjpoint = glm::vec2(trajectory_positions_blend[i].x, trajectory_positions_blend[i].z);
			if(glm::length(trjpoint - ((areas->wall_start[j] + areas->wall_stop[j]) / 2.0f)) >
			   glm::length(areas->wall_start[j] - areas->wall_stop[j]))
			{
				continue;
			}
			glm::vec2 segpoint = segment_nearest(areas->wall_start[j], areas->wall_stop[j], trjpoint);
			float segdist = glm::length(segpoint - trjpoint);
			if(segdist < areas->wall_width[j] + 100.0)
			{
				glm::vec2 prjpoint0 = (areas->wall_width[j] + 0.0f) * glm::normalize(trjpoint - segpoint) + segpoint;
				glm::vec2 prjpoint1 = (areas->wall_width[j] + 100.0f) * glm::normalize(trjpoint - segpoint) + segpoint;
				glm::vec2 prjpoint = glm::mix(prjpoint0, prjpoint1, glm::clamp((segdist - areas->wall_width[j]) / 100.0f, 0.0f, 1.0f));
				trajectory_positions_blend[i].x = prjpoint.x;
				trajectory_positions_blend[i].z = prjpoint.y;
			}
		}
		*/

		Directions[i] = MixDirections(Directions[i], TargetDirection, ScaleDirection);

		Heights[i] = Heights[halfLength];

		GaitStand[i] = GaitStand[halfLength];
		GaitWalk[i] = GaitWalk[halfLength];
		GaitJog[i] = GaitJog[halfLength];
		GaitJump[i] = GaitJump[halfLength];
		GaitBump[i] = GaitBump[halfLength];
		GaitCrouch[i] = GaitCrouch[halfLength];
	}

	for(int i = halfLength + 1; i < LENGTH; i++)
		Positions[i] = TrajectoryPositionsBlend[i];
}

void UTrajectoryComponent::TickRotations()
{
	const auto v = glm::vec3(0.0f, 1.0f, 0.0f);
	for(int i = 0; i < LENGTH; i++)
		Rotations[i] = glm::mat3(glm::rotate(atan2f(Directions[i].x, Directions[i].z), v));
}

void UTrajectoryComponent::TickHeights()
{
	/*
	
	for(int i = Trajectory::LENGTH / 2; i < Trajectory::LENGTH; i++)
	{
		trajectory->positions[i].y = heightmap->sample(glm::vec2(trajectory->positions[i].x, trajectory->positions[i].z));
	}

	trajectory->heights[Trajectory::LENGTH / 2] = 0.0;
	for(int i = 0; i < Trajectory::LENGTH; i += 10)
	{
		trajectory->heights[Trajectory::LENGTH / 2] += (trajectory->positions[i].y / ((Trajectory::LENGTH) / 10));
	}

	glm::vec3 root_position = glm::vec3(
		trajectory->positions[Trajectory::LENGTH / 2].x,
		trajectory->heights[Trajectory::LENGTH / 2],
		trajectory->positions[Trajectory::LENGTH / 2].z);

	glm::mat3 root_rotation = trajectory->rotations[Trajectory::LENGTH / 2];
	
	*/



	const float DistanceOffsetHeight = 100.f;
	const float DistanceOffsetFloor = 450.f;
	//Trajectory height
	for(int i = LENGTH / 2; i < LENGTH; i++)
	{
		FCollisionQueryParams CollisionParams = FCollisionQueryParams(FName(TEXT("GroundGeometryTrace")), true, OwnerPawn);
		CollisionParams.AddIgnoredActor(GetOwner());

		FHitResult OutResult;

		const auto StartPoint = Positions[i] * 100.f;
		const auto EndPoint = Positions[i] * 100.f;

		FVector UStartPoint = UPFNNHelperFunctions::XYZTranslationToXZY(StartPoint);
		FVector UEndPoint = UPFNNHelperFunctions::XYZTranslationToXZY(EndPoint);

		UStartPoint.Z = OwnerPawn->GetActorLocation().Z + DistanceOffsetHeight;
		UEndPoint.Z = OwnerPawn->GetActorLocation().Z - DistanceOffsetFloor;

		if(OwnerPawn->GetWorld()->LineTraceSingleByChannel(OutResult, UStartPoint, UEndPoint, ECollisionChannel::ECC_WorldStatic, CollisionParams))
		{
			Positions[i].y = OutResult.Location.Z * 0.01f;
		}
	}
	//Convert position to world space


	//for (int i = 0; i < LENGTH; i += 10)
	//{
	//	Heights[LENGTH / 2] += (Positions[i].y / ((LENGTH) / 10));
	//}
}

void UTrajectoryComponent::ResetTrajectory(const glm::vec3& arg_RootPosition, const glm::mat3& arg_RootRotation)
{
	for(int i = 0; i < LENGTH; i++)
	{
		Positions[i] = arg_RootPosition;
		Rotations[i] = arg_RootRotation;
		Directions[i] = glm::vec3(0, 0, 1);
		Heights[i] = arg_RootPosition.y;
		GaitStand[i] = 0.0;
		GaitWalk[i] = 0.0;
		GaitJog[i] = 0.0;
		GaitBump[i] = 0.0;
		GaitCrouch[i] = 0.0;
		GaitJump[i] = 0.0;
	}
}

void UTrajectoryComponent::UpdatePastTrajectory()
{
	for(int i = 0; i < LENGTH / 2; i++)
	{
		Positions[i] = Positions[i + 1];
		Directions[i] = Directions[i + 1];
		Rotations[i] = Rotations[i + 1];
		Heights[i] = Heights[i + 1];
		GaitStand[i] = GaitStand[i + 1];
		GaitWalk[i] = GaitWalk[i + 1];
		GaitJog[i] = GaitJog[i + 1];
		GaitBump[i] = GaitBump[i + 1];
	}
}

void UTrajectoryComponent::UpdateCurrentTrajectory(const float StandAmount, Eigen::ArrayXf& PFNN_Yp)
{
	const int32 halfLength = UTrajectoryComponent::LENGTH / 2;

	const glm::vec3 TrajectoryUpdate =  Rotations[halfLength] * glm::vec3(PFNN_Yp(0), 0, PFNN_Yp(1)); //TODEBUG: Rot
	 Positions[halfLength] =  Positions[halfLength] + StandAmount * TrajectoryUpdate;
	// Directions[halfLength] = UPFNNHelperFunctions::XZYTranslationToXYZ(glm::vec3( GetOwner()->GetActorForwardVector().X,  GetOwner()->GetActorForwardVector().Y, 0.0f));
	 Directions[halfLength] = glm::mat3(glm::rotate(StandAmount * -PFNN_Yp(2), glm::vec3(0, 1, 0))) *  Directions[halfLength]; //TODEBUG: Rot
	 Rotations[halfLength] = glm::mat3(glm::rotate(atan2f( Directions[halfLength].x,  Directions[halfLength].z), glm::vec3(0, 1, 0)));
}

void UTrajectoryComponent::UpdateFutureTrajectory(Eigen::ArrayXf& PFNN_Yp)
{
	const int32 halfLength  = UTrajectoryComponent::LENGTH / 2;
	const int32 W = halfLength / 10;
	const int32 FirstFutureNode = halfLength + 1;
	const auto trajectoryFeatureCalc = [&PFNN_Yp, &W](const auto M, const auto i, const auto ft) -> float
	{
		const auto iYp = 8 + (W * ft) + (i / 10) - W;
		return (1 - M) * PFNN_Yp(iYp) + M * PFNN_Yp(iYp + 1);
	};
	for(int32 i = FirstFutureNode; i < UTrajectoryComponent::LENGTH; i++)
	{
		const float M = fmod((static_cast<float>(i) - (halfLength)) / 10.0, 1.0);

		Positions[i].x = trajectoryFeatureCalc(M, i, 0);
		Positions[i].z = trajectoryFeatureCalc(M, i, 1);
		Directions[i].x = trajectoryFeatureCalc(M, i, 2);
		Directions[i].z = trajectoryFeatureCalc(M, i, 3);

		Positions[i] = (Rotations[halfLength] * Positions[i]) + Positions[halfLength];
		Directions[i] = glm::normalize(Rotations[halfLength] * Directions[i]);
		Rotations[i] = glm::mat3(glm::rotate(atan2f(Directions[i].x, Directions[i].z), glm::vec3(0, 1, 0)));
	}
}

glm::vec3 UTrajectoryComponent::MixDirections(const glm::vec3 arg_XDirection
											  , const glm::vec3 arg_YDirection
											  , const float arg_Scalar)
{
	const glm::quat XQuat = glm::angleAxis(atan2f(arg_XDirection.x, arg_XDirection.z), glm::vec3(0.0f, 1.0f, 0.0f));
	const glm::quat YQuat = glm::angleAxis(atan2f(arg_YDirection.x, arg_YDirection.z), glm::vec3(0.0f, 1.0f, 0.0f));
	const glm::quat ZQuat = glm::slerp(XQuat, YQuat, arg_Scalar);
	return ZQuat * glm::vec3(0.0f, 0.0f, 1.0f);
}


//USES NATIVE CPP TO ENSURE IT CAN BE USED IN REFERENCE PROJECT
void UTrajectoryComponent::LogTrajectoryData(int arg_FrameCount)
{
	try
	{
		FString RelativePath = FPaths::ProjectDir();
		const FString FullPath = RelativePath += "PFNN_Trajectory.log";

		std::fstream fs;
		fs.open(*FullPath, std::ios::out);
		if(fs.is_open())
		{
			fs << "UE4_Implementation" << std::endl;
			fs << "TrajectoryLog Frame[" << arg_FrameCount << "]" << std::endl << std::endl;

			fs << "#Basic Variables" << std::endl;
			fs << "TargetDirection: " << TargetDirection.x << "X, " << TargetDirection.y << "Y, " << TargetDirection.z << "Z" << std::endl;
			fs << "TargetVelocity:  " << TargetVelocity.x << "X, " << TargetVelocity.y << "Y, " << TargetVelocity.z << "Z" << std::endl;
			fs << "Width:           " << Width << std::endl;
			fs << "StrafeAmount:    " << StrafeAmount << std::endl;
			fs << "StrafeTarget:    " << StrafeTarget << std::endl;
			fs << "Responsive:      " << Responsive << std::endl;
			fs << "#End Basic Variables" << std::endl << std::endl;

			fs << "#Extra Smoothing values" << std::endl;
			fs << "	ExtraVelocitySmooth:	" << ExtraVelocitySmooth << std::endl;
			fs << "	ExtraDirectionSmooth:   " << ExtraDirectionSmooth << std::endl;
			fs << "	ExtraStrafeVelocity:	" << ExtraStrafeVelocity << std::endl;
			fs << "	ExtraStrafeSmooth:      " << ExtraStrafeSmooth << std::endl;
			fs << "	ExtraGaitSmooth:        " << ExtraGaitSmooth << std::endl;
			fs << "	ExtraJointSmooth:       " << ExtraJointSmooth << std::endl;
			fs << "#End Extra Smoothing values" << std::endl << std::endl;

			fs << "#Positional Data" << std::endl;
			for(size_t i = 0; i < LENGTH; i++)
			{
				fs << "TrajectoryNode[" << i << "]" << std::endl;
				fs << " Position:  " << Positions[i].x << "X, " << Positions[i].y << "Y, " << Positions[i].z << "Z" << std::endl;
				fs << "	Direction: " << Directions[i].x << "X, " << Directions[i].y << "Y, " << Directions[i].z << "Z" << std::endl;
				for(size_t x = 0; x < 3; x++)
				{
					fs << "	Rotation:  " << Rotations[i][x].x << "X, " << Rotations[i][x].y << ", " << Rotations[i][x].z << std::endl;
				}
			}
			fs << "#End Positional Data" << std::endl << std::endl;

			fs << "#Gaits" << std::endl;
			for(size_t i = 0; i < LENGTH; i++)
			{
				fs << "Gait[" << i << "]" << std::endl;
				fs << "	GaitStand: " << GaitStand[i] << std::endl;
				fs << "	GaitWalk:  " << GaitWalk[i] << std::endl;
				fs << "	GaitJog:   " << GaitJog[i] << std::endl;
				fs << "	GaitJump:  " << GaitJump[i] << std::endl;
				fs << "	GaitBump:  " << GaitBump[i] << std::endl;
			}
			fs << "#End Gaits" << std::endl << std::endl;
		}
		else
		{
			throw std::exception();
		}
		fs.close();
		return;
	}
	catch(std::exception e)
	{
#ifdef WITH_EDITOR
		UE_LOG(LogTemp, Error, TEXT("Failed to Log Trajectory output data"));
#endif
	}
}

void UTrajectoryComponent::TickTrajectory()
{
	TickInput();
	CalculateTargetDirection();
	TickGaits();
	PredictFutureTrajectory();
	TickRotations();
	TickHeights();
}

void UTrajectoryComponent::TickInput()
{
	if(APFNNCharacter* PFNNCharacter = Cast<APFNNCharacter>(GetOwner()))
	{
		UPawnMovementComponent* MovementComponent = PFNNCharacter->GetMovementComponent();
		glm::vec3 FlippedVelocity = UPFNNHelperFunctions::XZYTranslationToXYZ(MovementComponent->Velocity);
		CurrentFrameInput = glm::normalize(glm::vec2(FlippedVelocity.x, FlippedVelocity.z));
	}

	if(glm::abs(CurrentFrameInput.x) + glm::abs(CurrentFrameInput.y) < 0.305f)
		CurrentFrameInput = glm::vec2(0.0f);
}

void UTrajectoryComponent::CalculateTargetDirection()
{
	const auto actorForwardVector = OwnerPawn->GetActorForwardVector();

	glm::vec3 FlippedForward = UPFNNHelperFunctions::XZYTranslationToXYZ(glm::vec3(actorForwardVector.X, actorForwardVector.Y, 0.0f));
	glm::vec3 TrajectoryTargetDirectionNew = glm::normalize(FlippedForward);
	const glm::mat3 TrajectoryTargetRotation = glm::mat3(glm::rotate(atan2f(TrajectoryTargetDirectionNew.x, TrajectoryTargetDirectionNew.z), glm::vec3(0.0f, 1.0f, 0.0f)));

	const float currentWalkingSpeed = 7.5f;
	const float maxWalkSpeed = 500;
	const float TargetVelocitySpeed = OwnerPawn->GetVelocity().SizeSquared() / (pow(maxWalkSpeed/*OwnerPawn->GetMovementComponent()->GetMaxSpeed()*/, 2) * currentWalkingSpeed); //7.5 is current training walking speed

	const glm::vec3 TrajectoryTargetVelocityNew = TargetVelocitySpeed * (TrajectoryTargetRotation * /*glm::vec3(x_vel / 32768.0, 0, y_vel / 32768.0)*/ glm::vec3(CurrentFrameInput.x, 0.0f, CurrentFrameInput.y));
	TargetVelocity = glm::mix(TargetVelocity, TrajectoryTargetVelocityNew, ExtraVelocitySmooth);

	//StrafeTarget = ((SDL_JoystickGetAxis(stick, GAMEPAD_TRIGGER_L) / 32768.0) + 1.0) / 2.0; // TODO: strafe target
	StrafeAmount = glm::mix(StrafeAmount, StrafeTarget, ExtraStrafeSmooth);

	const glm::vec3 TrajectoryTargetVelocityDirection = glm::length(TargetVelocity) < 1e-05 ? TargetDirection : glm::normalize(TargetVelocity);
	TrajectoryTargetDirectionNew = MixDirections(TrajectoryTargetVelocityDirection, TrajectoryTargetDirectionNew, StrafeAmount);
	TargetDirection = MixDirections(TargetDirection, TrajectoryTargetDirectionNew, ExtraDirectionSmooth);

#pragma region Debug
	if(!bIsTrajectoryDebuggingEnabled)
		return;

	FVector FlippedCurrentFrameInput = UPFNNHelperFunctions::XYZTranslationToXZY(glm::vec3(CurrentFrameInput.x, 0.0f, CurrentFrameInput.y));
	FVector FlippedTargetDirectionNew = UPFNNHelperFunctions::XYZTranslationToXZY(TrajectoryTargetDirectionNew);
	FVector FlippedTargetDirection = UPFNNHelperFunctions::XYZTranslationToXZY(TargetDirection);
	FVector FlippedTargetVelocityNew = UPFNNHelperFunctions::XYZTranslationToXZY(TrajectoryTargetVelocityNew);
	FVector FlippedTargetVelocity = UPFNNHelperFunctions::XYZTranslationToXZY(TargetVelocity);

	GEngine->AddOnScreenDebugMessage(-1, 0.0f, FColor::White, FString::Printf(TEXT("%s Current frame input"), *FlippedCurrentFrameInput.ToString()));
	GEngine->AddOnScreenDebugMessage(-1, 0.0f, FColor::Blue, FString::Printf(TEXT("%s TrajectoryTargetDirectionNew"), *FlippedTargetDirectionNew.ToString()));
	GEngine->AddOnScreenDebugMessage(-1, 0.0f, FColor::Green, FString::Printf(TEXT("%s TargetDirection"), *FlippedTargetDirection.ToString()));
	GEngine->AddOnScreenDebugMessage(-1, 0.0f, FColor::Red, FString::Printf(TEXT("%s TargetVelocityNew"), *FlippedTargetVelocityNew.ToString()));
	GEngine->AddOnScreenDebugMessage(-1, 0.0f, FColor::Yellow, FString::Printf(TEXT("%s TargetVelocity"), *FlippedTargetVelocity.ToString()));

	GEngine->AddOnScreenDebugMessage(-1, 0.0f, FColor::Yellow, FString::Printf(TEXT("%s Pawn velocity vector"), *OwnerPawn->GetVelocity().ToString()));
	const auto SizeSquared = OwnerPawn->GetVelocity().SizeSquared();
	FString stri = FString::SanitizeFloat(SizeSquared, 8);
	GEngine->AddOnScreenDebugMessage(-1, 0.0f, FColor::Yellow, FString::Printf(TEXT("%s Pawn velocity Squared"),  *stri));

#if 0 // unexpect throws
	FVector ActorLoc = GetOwner()->GetActorLocation();
	const auto drawDebugArrow = [&](const auto& actor, const auto& appendix, const auto color)
	{
		const float lifeTime = -1.f;
		const uint8 depthPrior = 0;
		const float thickness = 2.f;
		const bool bPersistentLine = false;
		DrawDebugDirectionalArrow(GetWorld(), actor, actor + appendix + rand() % 5, 0.0f, color, bPersistentLine, lifeTime, depthPrior, thickness);
	};

	FVector FrameInputActor = ActorLoc + FVector(0.0f, 0.0f, 200.0f);
	drawDebugArrow(FrameInputActor, FlippedCurrentFrameInput * 100, FColor::White);

	FVector TargetDirectionActor = ActorLoc + FVector(0.0f, 0.0f, 250.0f);
	drawDebugArrow(TargetDirectionActor, FlippedTargetDirectionNew * 100.0f, FColor::Blue);
	drawDebugArrow(TargetDirectionActor, FlippedTargetDirection * 100.0f, FColor::Green);

	FVector TargetVelocityActor = ActorLoc + FVector(0.0f, 0.0f, 225.0f);
	drawDebugArrow(TargetVelocityActor, FlippedTargetVelocityNew * 1000.0f, FColor::Red);
	drawDebugArrow(TargetVelocityActor, FlippedTargetVelocity * 1000.0f, FColor::Yellow);
#endif // 0
#pragma endregion
}

void UTrajectoryComponent::DrawDebugTrajectory()
{
	if(!bIsTrajectoryDebuggingEnabled)
		return;

	const glm::vec3 StartingPoint = Positions[0] * 100.0f;			//Get the leading point of the trajectory
	const glm::vec3 MidPoint = Positions[LENGTH / 2] * 100.0f;		//Get the mid point/player point of the trajectory
	const glm::vec3 EndingPoint = Positions[LENGTH - 1] * 100.0f;	//Get the ending point of the trajectory

	for(size_t i = 0; i < LENGTH; i++)
	{
		if(i % 10 == 0 || i == LENGTH - 1)
		{
			FVector DebugLocation = (UPFNNHelperFunctions::XYZTranslationToXZY(Positions[i]) * 100.0f);
			DrawDebugPoint(GetWorld(), DebugLocation, 7.5f, FColor::Red);

			FVector CrossProduct = FVector::CrossProduct(FVector(0.0f, 0.0f, 1.0f), UPFNNHelperFunctions::XYZTranslationToXZY(Directions[i])) * 50.0f;

			DrawDebugPoint(GetWorld(), DebugLocation + CrossProduct, 10.0f, FColor::Black);
			DrawDebugPoint(GetWorld(), DebugLocation - CrossProduct, 10.0f, FColor::Black);

		}
	}

	FVector DebugStartingPoint = UPFNNHelperFunctions::XYZTranslationToXZY(StartingPoint);
	FVector DebugMidPoint = UPFNNHelperFunctions::XYZTranslationToXZY(MidPoint);
	FVector DebugEndPoint = UPFNNHelperFunctions::XYZTranslationToXZY(EndingPoint);

	DrawDebugPoint(GetWorld(), DebugStartingPoint, 10.0f, FColor::Red);
	DrawDebugPoint(GetWorld(), DebugMidPoint, 10.0f, FColor::Red);
	DrawDebugPoint(GetWorld(), DebugEndPoint, 10.0f, FColor::Red);

	DrawDebugString(GetWorld(), DebugStartingPoint, TEXT("Past"), nullptr, FColor::Blue, 0.001f);
	DrawDebugString(GetWorld(), DebugMidPoint, TEXT("Current"), nullptr, FColor::Blue, 0.001f);
	DrawDebugString(GetWorld(), DebugEndPoint, TEXT("Future"), nullptr, FColor::Blue, 0.001f);

	const FVector StartingDirection = UPFNNHelperFunctions::XYZTranslationToXZY(Directions[0]);
	const FVector MidDirection = UPFNNHelperFunctions::XYZTranslationToXZY(Directions[LENGTH / 2]);
	const FVector EndDirection = UPFNNHelperFunctions::XYZTranslationToXZY(Directions[LENGTH - 1]);

	GEngine->AddOnScreenDebugMessage(-1, 0.0f, FColor::Black, FString::Printf(TEXT("%s Past Direction"), *StartingDirection.ToString()));
	GEngine->AddOnScreenDebugMessage(-1, 0.0f, FColor::Purple, FString::Printf(TEXT("%s Past Position"), *DebugStartingPoint.ToString()));
	GEngine->AddOnScreenDebugMessage(-1, 0.0f, FColor::Black, FString::Printf(TEXT("%s Present Direction"), *MidDirection.ToString()));
	GEngine->AddOnScreenDebugMessage(-1, 0.0f, FColor::Purple, FString::Printf(TEXT("%s Present Position"), *DebugMidPoint.ToString()));
	GEngine->AddOnScreenDebugMessage(-1, 0.0f, FColor::Black, FString::Printf(TEXT("%s Future Direction"), *EndDirection.ToString()));
	GEngine->AddOnScreenDebugMessage(-1, 0.0f, FColor::Purple, FString::Printf(TEXT("%s Future Position"), *DebugEndPoint.ToString()));
}

