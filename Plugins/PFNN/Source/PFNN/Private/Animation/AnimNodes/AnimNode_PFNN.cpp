

#include "Animation/AnimNodes/AnimNode_PFNN.h"
#include "Animation/AnimInstances/PFNNAnimInstance.h"
#include "Animation/AnimComponents/TrajectoryComponent.h"
#include "Core/Character/PFNNCharacter.h"
#include "Utilities/PFNNHelperFunctions.h"

#include "Animation/AnimInstanceProxy.h"
#include "Animation/PFNN/PhaseFunctionNeuralNetwork.h"
#include "DrawDebugHelpers.h"
#include "Engine/Engine.h"
#include "HAL/PlatformFileManager.h"

#define GLM_ENABLE_EXPERIMENTAL
#include <ThirdParty/glm/glm.hpp>
#include <ThirdParty/glm/gtx/transform.hpp>
#include <ThirdParty/glm/gtx/euler_angles.hpp>

#include <fstream>

UPhaseFunctionNeuralNetwork* FAnimNode_PFNN::PFNN = nullptr;

FAnimNode_PFNN::FAnimNode_PFNN(): PFNNAnimInstance(nullptr), Trajectory(nullptr), Phase(0),  FrameCounter(0),
								  bIsPFNNLoaded(false)
{
}

void FAnimNode_PFNN::LoadData()
{
	LoadXForms();
	LoadPFNN();
}

void FAnimNode_PFNN::LoadXForms()
{
	IPlatformFile& PlatformFile = FPlatformFileManager::Get().GetPlatformFile();

	const FString RelativePath = FPaths::ProjectDir();
	const FString FullPathParents = RelativePath + FString::Printf(TEXT("Plugins/PFNN/Content/MachineLearning/PhaseFunctionNeuralNetwork/character_parents.bin"));
	const FString FullPathXforms = RelativePath + FString::Printf(TEXT("Plugins/PFNN/Content/MachineLearning/PhaseFunctionNeuralNetwork/character_xforms.bin"));

	IFileHandle* FileHandle = PlatformFile.OpenRead(*FullPathParents);
	if (FileHandle == nullptr)
	{
		UE_LOG(PFNN_Logging, Error, TEXT("Fatal error, Failed to load charater parents"));
		return;
	}
	float JointParentsFloat[JOINT_NUM];
	FileHandle->Read(reinterpret_cast<uint8*>(JointParentsFloat), sizeof(JointParentsFloat));

	for (int i = 0; i < JOINT_NUM; i++)
	{
		JointParents[i] = static_cast<int>(JointParentsFloat[i]);
	}


	FileHandle = PlatformFile.OpenRead(*FullPathXforms);
	if (FileHandle == nullptr)
	{
		UE_LOG(PFNN_Logging, Fatal, TEXT("Fatal error, Failed to load character xforms"));
		return;
	}
	FileHandle->Read(reinterpret_cast<uint8*>(JointRestXform), sizeof(JointRestXform));

	for (int i = 0; i < JOINT_NUM; i++)
	{
		JointRestXform[i] = glm::transpose(JointRestXform[i]);
	}

	delete FileHandle;
}

void FAnimNode_PFNN::LoadPFNN()
{
	if(!bIsPFNNLoaded && Trajectory)
	{
		if (PFNN == nullptr)
		{
			PFNN = NewObject<UPhaseFunctionNeuralNetwork>();
		}

		bIsPFNNLoaded = PFNN->LoadNetworkData(Trajectory->GetOwner());
	}
}

void FAnimNode_PFNN::ApplyPFNN()
{
	Trajectory->TickTrajectory();

	glm::vec3 RootPosition = Trajectory->GetRootPosition();
	glm::mat3 RootRotation = Trajectory->GetRootRotation();

	//Input trajectiory positions and directions
	for (int i = 0; i < UTrajectoryComponent::LENGTH; i += 10)
	{
		int w = UTrajectoryComponent::LENGTH / 10;
		const glm::vec3 Position = glm::inverse(RootRotation) * (Trajectory->Positions[i] - RootPosition);
		const glm::vec3 Direction = glm::inverse(RootRotation) * Trajectory->Directions[i];
		PFNN->Xp((w * 0) + i / 10) = Position.x;
		PFNN->Xp((w * 1) + i / 10) = Position.z;
		PFNN->Xp((w * 2) + i / 10) = Direction.x;
		PFNN->Xp((w * 3) + i / 10) = Direction.z;
	}

	// Input trajectory gaits
	for (int i = 0; i < UTrajectoryComponent::LENGTH; i += 10)
	{
		const int w = UTrajectoryComponent::LENGTH / 10;
		PFNN->Xp((w * 4) + i / 10) = Trajectory->GaitStand[i];
		PFNN->Xp((w * 5) + i / 10) = Trajectory->GaitWalk[i];
		PFNN->Xp((w * 6) + i / 10) = Trajectory->GaitJog[i];
		PFNN->Xp((w * 7) + i / 10) = 0; //Unused input for crouch?;
		PFNN->Xp((w * 8) + i / 10) = Trajectory->GaitJump[i];
		PFNN->Xp((w * 9) + i / 10) = 0; //Unused input
	}

	//Input previous join position / velocity / rotations
	const glm::vec3 PreviousRootPosition = Trajectory->GetPreviousRootPosition();
	const glm::mat3 PreviousRootRotation = Trajectory->GetPreviousRootRotation();

	for (int i = 0; i < JOINT_NUM; i++)
	{
		const int o = ((UTrajectoryComponent::LENGTH / 10)*10);
		const glm::vec3 Position = glm::inverse(PreviousRootRotation) * (JointPosition[i] - PreviousRootPosition);
		const glm::vec3 Previous = glm::inverse(PreviousRootRotation) * JointVelocitys[i];
		//Magical numbers are indexes for the PFNN
		PFNN->Xp(o + (JOINT_NUM * 3 * 0) + i * 3 + 0) = Position.x;
		PFNN->Xp(o + (JOINT_NUM * 3 * 0) + i * 3 + 1) = Position.y;
		PFNN->Xp(o + (JOINT_NUM * 3 * 0) + i * 3 + 2) = Position.z;
		PFNN->Xp(o + (JOINT_NUM * 3 * 1) + i * 3 + 0) = Previous.x;
		PFNN->Xp(o + (JOINT_NUM * 3 * 1) + i * 3 + 1) = Previous.y;
		PFNN->Xp(o + (JOINT_NUM * 3 * 1) + i * 3 + 2) = Previous.z;
	}

	//Input heights for the trajectory
	for (int i = 0; i < UTrajectoryComponent::LENGTH; i += 10)
	{
		const int o = (((UTrajectoryComponent::LENGTH) / 10)*10) + JOINT_NUM * 3 * 2;
		const int w = UTrajectoryComponent::LENGTH / 10;

		const float DistanceOffsetHeight = 100.f;
		const float DistanceOffsetFloor = 450.f;
		FCollisionQueryParams CollisionParams = FCollisionQueryParams(FName(TEXT("GroundGeometryTrace")), true, Trajectory->GetOwner());
		CollisionParams.AddIgnoredActor(Trajectory->GetOwner());

		FHitResult LeftOutResult;
		FHitResult RightOutResult;

		const auto LeftStartPoint = Trajectory->Positions[i] * 100.f + (Trajectory->Rotations[i] * glm::vec3(Trajectory->Width));
		const auto RightStartPoint = Trajectory->Positions[i] * 100.f + (Trajectory->Rotations[i] * glm::vec3(-Trajectory->Width));

		FVector ULeftStartPoint = UPFNNHelperFunctions::XYZTranslationToXZY(LeftStartPoint);
		FVector ULeftEndPoint = UPFNNHelperFunctions::XYZTranslationToXZY(LeftStartPoint);
		FVector URightStartPoint = UPFNNHelperFunctions::XYZTranslationToXZY(RightStartPoint);
		FVector URightEndPoint = UPFNNHelperFunctions::XYZTranslationToXZY(RightStartPoint);

		ULeftStartPoint.Z	= Trajectory->GetOwner()->GetActorLocation().Z + DistanceOffsetHeight;
		ULeftEndPoint.Z		= Trajectory->GetOwner()->GetActorLocation().Z - DistanceOffsetFloor;
		URightStartPoint.Z	= Trajectory->GetOwner()->GetActorLocation().Z + DistanceOffsetHeight;
		URightEndPoint.Z	= Trajectory->GetOwner()->GetActorLocation().Z - DistanceOffsetFloor;

		Trajectory->GetOwner()->GetWorld()->LineTraceSingleByChannel(
			LeftOutResult, 
			ULeftStartPoint, 
			ULeftEndPoint, 
			ECollisionChannel::ECC_WorldStatic, 
			CollisionParams);

		Trajectory->GetOwner()->GetWorld()->LineTraceSingleByChannel(
			RightOutResult, 
			URightStartPoint, 
			URightEndPoint, 
			ECollisionChannel::ECC_WorldStatic, 
			CollisionParams);

		glm::vec3 LeftResultLocation = UPFNNHelperFunctions::XZYTranslationToXYZ(LeftOutResult.Location);
		glm::vec3 RightResultLocation = UPFNNHelperFunctions::XZYTranslationToXYZ(RightOutResult.Location);

		//DrawDebugLine(Trajectory->GetOwner()->GetWorld(), ULeftStartPoint, ULeftEndPoint, FColor::Red, false, 0.01f, 0, 1);
		//DrawDebugLine(Trajectory->GetOwner()->GetWorld(), URightStartPoint, URightEndPoint, FColor::Red, false, 0.01f, 0, 1);

		PFNN->Xp(o + (w * 0) + (i / 10)) = LeftResultLocation.y * 0.01f;
		PFNN->Xp(o + (w * 1) + (i / 10)) = Trajectory->Positions[i].y * 0.01f;
		PFNN->Xp(o + (w * 2) + (i / 10)) = RightResultLocation.y * 0.01f;
	}

	PFNN->Predict(Phase);

	//Build local transformation for the joints
	for (int i = 0; i < JOINT_NUM; i++)
	{
		const int OPosition = 8 + (((UTrajectoryComponent::LENGTH / 2) / 10) * 4) + JOINT_NUM * 3 * 0;
		const int OVelocity = 8 + (((UTrajectoryComponent::LENGTH / 2) / 10) * 4) + JOINT_NUM * 3 * 1;
		const int ORoation = 8 + (((UTrajectoryComponent::LENGTH / 2) / 10) * 4) + JOINT_NUM * 3 * 2;

		const glm::vec3 Position = (RootRotation * glm::vec3(PFNN->Yp(OPosition + i * 3 + 0), PFNN->Yp(OPosition + i * 3 + 1), PFNN->Yp(OPosition + i * 3 + 2))) + RootPosition;
		const glm::vec3 Velocity = (RootRotation * glm::vec3(PFNN->Yp(OVelocity + i * 3 + 0), PFNN->Yp(OVelocity + i * 3 + 1), PFNN->Yp(OVelocity + i * 3 + 2)));
		const glm::vec3 Rotation = glm::vec3(PFNN->Yp(ORoation + i * 3 + 0), PFNN->Yp(ORoation + i * 3 + 1), PFNN->Yp(ORoation + i * 3 + 2));

		JointPosition[i] = glm::mix(JointPosition[i] + Velocity, Position, Trajectory->ExtraJointSmooth);
		JointVelocitys[i] = Velocity;
		JointRotations[i] = Rotation;

	}
	//Forward kinematics
	for (int i = 0; i < JOINT_NUM; i++)
	{
		JointGlobalAnimXform[i] = JointAnimXform[i];
		JointGlobalRestXform[i] = JointRestXform[i];
		int j = JointParents[i];
		while (j != -1)
		{
			JointGlobalAnimXform[i] = JointAnimXform[j] * JointGlobalAnimXform[i];
			JointGlobalRestXform[i] = JointRestXform[j] * JointGlobalRestXform[i];
			j = JointParents[j];
		}
		JointMeshXform[i] = JointGlobalAnimXform[i] * glm::inverse(JointGlobalRestXform[i]);
	}

	Trajectory->UpdatePastTrajectory();

	Trajectory->Positions[UTrajectoryComponent::LENGTH / 2] = RootPosition;

	//Update current trajectory
	float StandAmount = powf(1.0f - Trajectory->GaitStand[UTrajectoryComponent::LENGTH / 2], 0.25f);

	const glm::vec3 TrajectoryUpdate = Trajectory->Rotations[UTrajectoryComponent::LENGTH / 2] * glm::vec3(PFNN->Yp(0), 0, PFNN->Yp(1)); //TODEBUG: Rot
	Trajectory->Positions[UTrajectoryComponent::LENGTH / 2] = Trajectory->Positions[UTrajectoryComponent::LENGTH / 2];// + StandAmount * TrajectoryUpdate;
	Trajectory->Directions[UTrajectoryComponent::LENGTH / 2] = UPFNNHelperFunctions::XZYTranslationToXYZ(glm::vec3(Trajectory->GetOwner()->GetActorForwardVector().X, Trajectory->GetOwner()->GetActorForwardVector().Y, 0.0f));
	Trajectory->Directions[UTrajectoryComponent::LENGTH / 2] = glm::mat3(glm::rotate(StandAmount * -PFNN->Yp(2), glm::vec3(0, 1, 0))) * Trajectory->Directions[UTrajectoryComponent::LENGTH / 2]; //TODEBUG: Rot
	Trajectory->Rotations[UTrajectoryComponent::LENGTH / 2] = glm::mat3(glm::rotate(atan2f(
		Trajectory->Directions[UTrajectoryComponent::LENGTH / 2].x,
		Trajectory->Directions[UTrajectoryComponent::LENGTH / 2].z), glm::vec3(0, 1, 0)));

	//TODO: Add wall logic

	//Update future trajectory
	int FirstFutureNode = UTrajectoryComponent::LENGTH / 2 + 1;
	for (int i = FirstFutureNode; i < UTrajectoryComponent::LENGTH; i++)
	{
		const int W = (UTrajectoryComponent::LENGTH / 2) / 10;
		const float M = fmod((static_cast<float>(i) - (UTrajectoryComponent::LENGTH / 2)) / 10.0, 1.0);

		Trajectory->Positions[i].x = (1 - M) * PFNN->Yp(8 + (W * 0) + (i / 10) - W) + M * PFNN->Yp(8 + (W * 0) + (i / 10) - W + 1); 
		Trajectory->Positions[i].z = (1 - M) * PFNN->Yp(8 + (W * 1) + (i / 10) - W) + M * PFNN->Yp(8 + (W * 1) + (i / 10) - W + 1); 
		Trajectory->Directions[i].x = (1 - M) * PFNN->Yp(8 + (W * 2) + (i / 10) - W) + M * PFNN->Yp(8 + (W * 2) + (i / 10) - W + 1);
		Trajectory->Directions[i].z = (1 - M) * PFNN->Yp(8 + (W * 3) + (i / 10) - W) + M * PFNN->Yp(8 + (W * 3) + (i / 10) - W + 1);

		Trajectory->Positions[i] = (Trajectory->Rotations[UTrajectoryComponent::LENGTH / 2] * Trajectory->Positions[i]) + Trajectory->Positions[UTrajectoryComponent::LENGTH / 2];
		Trajectory->Directions[i] = (Trajectory->Rotations[UTrajectoryComponent::LENGTH / 2]) * Trajectory->Directions[i];
		Trajectory->Rotations[i] = glm::mat3(glm::rotate(atan2f(Trajectory->Directions[i].x, Trajectory->Directions[i].z), glm::vec3(0, 1, 0)));
	}

	FinalBoneLocations.Empty();
	FinalBoneRotations.Empty();

	FinalBoneLocations.SetNum(JOINT_NUM);
	FinalBoneRotations.SetNum(JOINT_NUM);
	const float FinalScale = 1.0f;

	for (int32 i = 0; i < JOINT_NUM; i++)
	{
		FinalBoneLocations[i] = UPFNNHelperFunctions::XYZTranslationToXZY(JointPosition[i]);

		FVector UnrealJointRotation = UPFNNHelperFunctions::XYZTranslationToXZY(JointRotations[i]);
		FinalBoneRotations[i] = FQuat(FQuat::MakeFromEuler(FVector::RadiansToDegrees(UnrealJointRotation)));

	}
	
	//Phase update
	Phase = fmod(Phase + (StandAmount * 0.9f + 0.1f) * 2.0f * PI * PFNN->Yp(3), 2.0f * PI);

	FrameCounter++;
	if (FrameCounter >= 1)
	{
		Trajectory->LogTrajectoryData(FrameCounter);
		LogNetworkData(FrameCounter);
	}

	VisualizePhase();

}

glm::quat FAnimNode_PFNN::QuaternionExpression(const glm::vec3 arg_Vector)
{
	float W = glm::length(arg_Vector);

	const glm::quat Quat = W < 0.01 ? glm::quat(1.0f, 0.0f, 0.0f, 0.0f) : glm::quat(
		cosf(W),
		arg_Vector.x * (sinf(W) / W),
		arg_Vector.y * (sinf(W) / W),
		arg_Vector.z * (sinf(W) / W));

	return Quat / sqrtf(Quat.w*Quat.w + Quat.x*Quat.x + Quat.y*Quat.y + Quat.z*Quat.z);
}

UPFNNAnimInstance * FAnimNode_PFNN::GetPFNNInstanceFromContext(const FAnimationInitializeContext& arg_Context)
{
	FAnimInstanceProxy* AnimProxy = arg_Context.AnimInstanceProxy;
	if (AnimProxy)
	{
		return Cast<UPFNNAnimInstance>(AnimProxy->GetAnimInstanceObject());
	}
	return nullptr;
}

UPFNNAnimInstance * FAnimNode_PFNN::GetPFNNInstanceFromContext(const FAnimationUpdateContext & arg_Context)
{
	FAnimInstanceProxy* AnimProxy = arg_Context.AnimInstanceProxy;
	if (AnimProxy)
	{
		return Cast<UPFNNAnimInstance>(AnimProxy->GetAnimInstanceObject());
	}
	return nullptr;
}

void FAnimNode_PFNN::Initialize_AnyThread(const FAnimationInitializeContext& arg_Context)
{
	FAnimNode_Base::Initialize_AnyThread(arg_Context);
	
	PFNNAnimInstance = GetPFNNInstanceFromContext(arg_Context);
	if (!PFNNAnimInstance) 
	{
		UE_LOG(LogTemp, Error, TEXT("PFNN Animation node should only be added to a PFNNAnimInstance child class!"));
	}
}

void FAnimNode_PFNN::Update_AnyThread(const FAnimationUpdateContext& arg_Context)
{
	FAnimNode_Base::Update_AnyThread(arg_Context);

	if (!bIsPFNNLoaded)
	{
		LoadData();
	}

	if (PFNNAnimInstance) 
	{
		Trajectory = PFNNAnimInstance->GetOwningTrajectoryComponent();
	}
	
	if(Trajectory != nullptr && bIsPFNNLoaded)
		ApplyPFNN();


}

void FAnimNode_PFNN::Evaluate_AnyThread(FPoseContext& arg_Output)
{
	if (bIsPFNNLoaded)
	{
		const FTransform& CharacterTransform = arg_Output.AnimInstanceProxy->GetActorTransform();
		if (FinalBoneLocations.Num() >= JOINT_NUM && FinalBoneRotations.Num() >= JOINT_NUM)
		{
			auto Bones = arg_Output.Pose.GetBoneContainer();

			for (int32 i = 0; i < JOINT_NUM; i++)
			{
				const FCompactPoseBoneIndex CurrentBoneIndex(i);
				const FCompactPoseBoneIndex ParentBoneIndex(Bones.GetParentBoneIndex(CurrentBoneIndex));

				if (ParentBoneIndex.GetInt() == -1)
				{
					//Do nothing first UE4 root bone skips
					arg_Output.Pose[CurrentBoneIndex].SetRotation(FQuat::MakeFromEuler(FVector::DegreesToRadians(FVector(90.0f, 0.0f, 0.0f))));
				}
				else if(ParentBoneIndex.GetInt() == 0)
				{
					//Root Bone No conversion needed
					arg_Output.Pose[CurrentBoneIndex].SetRotation(FinalBoneRotations[CurrentBoneIndex.GetInt() - 1]);
					arg_Output.Pose[CurrentBoneIndex].SetLocation(FinalBoneLocations[CurrentBoneIndex.GetInt() - 1]);
					
				}
				else
				{	//Conversion to LocalSpace (hopefully)
					FTransform CurrentBoneTransform = FTransform(FinalBoneRotations[CurrentBoneIndex.GetInt() -1], FinalBoneLocations[CurrentBoneIndex.GetInt() -1], FVector::OneVector);
					FTransform ParentBoneTransform = FTransform(FinalBoneRotations[ParentBoneIndex.GetInt() -1], FinalBoneLocations[ParentBoneIndex.GetInt() -1], FVector::OneVector);

					FTransform LocalBoneTransform = CurrentBoneTransform.GetRelativeTransform(ParentBoneTransform);

					arg_Output.Pose[CurrentBoneIndex].SetComponents(LocalBoneTransform.GetRotation(), LocalBoneTransform.GetLocation(), LocalBoneTransform.GetScale3D());
					//arg_Output.Pose[CurrentBoneIndex].SetRotation(LocalBoneTransform.GetRotation());
					LocalBoneTransform.SetLocation(LocalBoneTransform.GetRotation().Inverse() * LocalBoneTransform.GetLocation());
				}
			}
			arg_Output.Pose.NormalizeRotations();
#ifdef WITH_EDITOR
			DrawDebugSkeleton(arg_Output);
			DrawDebugBoneVelocity(arg_Output);
#endif
		}
		else
		{
			UE_LOG(PFNN_Logging, Error, TEXT("PFNN results were not properly applied!"));
		}
	}
}

void FAnimNode_PFNN::LogNetworkData(int arg_FrameCounter) 
{
	try 
	{
		std::fstream fs;
		fs.open("UE5_Network.log", std::ios::out);

		if (fs.is_open()) 
		{
			fs << "UE4_Network" << std::endl;
			fs << "Network Frame[" << arg_FrameCounter << "]" << std::endl << std::endl;
			
			fs << "Current Phase: " << Phase << std::endl << std::endl;

			fs << "Joints" << std::endl;
			for (size_t i = 0; i < JOINT_NUM; i++)
			{
				fs << "Joint[" << i << "]" << std::endl;
				fs << "	JointPosition: " << JointPosition[i].x << "X, " << JointPosition[i].y << "Y, " << JointPosition[i].z << "Z" << std::endl;
				fs << "	JointVelocitys: " << JointVelocitys[i].x << "X, " << JointVelocitys[i].y << "Y, " << JointVelocitys[i].z << "Z" << std::endl;

				for (size_t x = 0; x < 3; x++)
				{
				//fs << "	JointRotations:  " << JointRotations[i][x].x << "X, " << JointRotations[i][x].y << ", " << JointRotations[i][x].z << std::endl;
				}

				for (size_t x = 0; x < 3; x++)
				{
					fs << "	JointAnimXform:  " << JointAnimXform[i][x].x << "X, " << JointAnimXform[i][x].y << ", " << JointAnimXform[i][x].z << std::endl;
				}

				for (size_t x = 0; x < 3; x++)
				{
					fs << "	JointRestXform:  " << JointRestXform[i][x].x << "X, " << JointRestXform[i][x].y << ", " << JointRestXform[i][x].z << std::endl;
				}

				for (size_t x = 0; x < 3; x++)
				{
					fs << "	JointMeshXform:  " << JointMeshXform[i][x].x << "X, " << JointMeshXform[i][x].y << ", " << JointMeshXform[i][x].z << std::endl;
				}

				for (size_t x = 0; x < 3; x++)
				{
					fs << "	JointGlobalRestXform:  " << JointGlobalRestXform[i][x].x << "X, " << JointGlobalRestXform[i][x].y << ", " << JointGlobalRestXform[i][x].z << std::endl;
				}

				for (size_t x = 0; x < 3; x++)
				{
					fs << "	JointGlobalAnimXform:  " << JointGlobalAnimXform[i][x].x << "X, " << JointGlobalAnimXform[i][x].y << ", " << JointGlobalAnimXform[i][x].z << std::endl;
				}

				fs << "JoinParents: " << JointParents[i] << std::endl;
			}
			fs << "End Joints" << std::endl << std::endl;

			fs << "FinalLocations" << std::endl;
			for (size_t i = 0; i < FinalBoneLocations.Num(); i++)
			{
				fs << "Bone[" << i << "]" << std::endl;
				fs << "	FinalBoneLocation: " << FinalBoneLocations[i].X << "X, " << FinalBoneLocations[i].Y << "Y, " << FinalBoneLocations[i].Z << "Z" << std::endl;
				fs << "	FinalBoneRotation: " << FinalBoneRotations[i].X << "X, " << FinalBoneRotations[i].Y << "Y, " << FinalBoneRotations[i].Z << "Z, " << FinalBoneRotations[i].W << "W"<< std::endl;
			}
			fs << "End FinalLocations" << std::endl;

		}
		else 
		{
			throw std::exception();
		}
	}
	catch (std::exception e) 
	{
#ifdef WITH_EDITOR
		UE_LOG(LogTemp, Log, TEXT("Failed to log network data"));
#endif
	}
	
}

void FAnimNode_PFNN::DrawDebugSkeleton(const FPoseContext& arg_Context) 
{
	APFNNCharacter* Character = Cast<APFNNCharacter>(Trajectory->GetOwner());
	if (Character)
	{
		if (Character->HasDebuggingEnabled())
		{
			const FTransform& CharacterTransform = arg_Context.AnimInstanceProxy->GetActorTransform();
			FBoneContainer Bones = arg_Context.Pose.GetBoneContainer();

			for (int32 i = 0; i < JOINT_NUM; i++)
			{
				const FCompactPoseBoneIndex CurrentBoneIndex(i);
				FCompactPoseBoneIndex ParentBoneIndex(Bones.GetParentBoneIndex(CurrentBoneIndex));

				FVector CurrentBoneLocation = arg_Context.Pose[CurrentBoneIndex].GetLocation();
				FVector ParentBoneLocation = CurrentBoneLocation;
				if (ParentBoneIndex.GetInt() != -1)
				{
					ParentBoneLocation = arg_Context.Pose[ParentBoneIndex].GetLocation();
				}

				while (ParentBoneIndex.GetInt() != -1)
				{
					CurrentBoneLocation += arg_Context.Pose[ParentBoneIndex].GetLocation();
					ParentBoneIndex = Bones.GetParentBoneIndex(ParentBoneIndex);
				}

				ParentBoneIndex = Bones.GetParentBoneIndex(CurrentBoneIndex);
				if (ParentBoneIndex.GetInt() != -1)
				{
					ParentBoneIndex = Bones.GetParentBoneIndex(ParentBoneIndex);

					while (ParentBoneIndex.GetInt() != -1)
					{
						ParentBoneLocation += arg_Context.Pose[ParentBoneIndex].GetLocation();
						ParentBoneIndex = Bones.GetParentBoneIndex(ParentBoneIndex);
					}
				}

				FRotator BoneRotator = FRotator(FinalBoneRotations[i]);
				arg_Context.AnimInstanceProxy->AnimDrawDebugCoordinateSystem(CurrentBoneLocation + CharacterTransform.GetLocation(), BoneRotator, 10.0f, false, -1.0f, 0.2);
				arg_Context.AnimInstanceProxy->AnimDrawDebugSphere(CurrentBoneLocation + CharacterTransform.GetLocation(), 2.5f, 12, FColor::Green, false, -1.0f);
				arg_Context.AnimInstanceProxy->AnimDrawDebugLine(CurrentBoneLocation + CharacterTransform.GetLocation(), ParentBoneLocation + CharacterTransform.GetLocation(), FColor::White, false, -1, 2.0f);

				if (Trajectory->GetOwner()->GetWorld() != nullptr)
					DrawDebugString(Trajectory->GetOwner()->GetWorld(), CurrentBoneLocation + CharacterTransform.GetLocation(), FString::FromInt(i), static_cast<AActor*>(0), FColor::Red, 0.01f, false, 2.0f);
			}
		}
	}
}

void FAnimNode_PFNN::DrawDebugBoneVelocity(const FPoseContext& arg_Context)
{
	if(!GEngine)
		return;
	
	APFNNCharacter* Character = Cast<APFNNCharacter>(Trajectory->GetOwner());
	if (Character)
	{
		if (Character->HasDebuggingEnabled())
		{
			for (int32 i = 0; i < JOINT_NUM; i++)
			{
				auto JointPos = FVector(JointPosition[i].x, JointPosition[i].z, JointPosition[i].y);

				arg_Context.AnimInstanceProxy->AnimDrawDebugLine(
					JointPos,
					JointPos - 10 * FVector(JointVelocitys[i].x, JointVelocitys[i].z, JointVelocitys[i].y),
					FColor::Yellow,
					false,
					-1,
					0.5f
				);

			}
		}
	}
}

void FAnimNode_PFNN::VisualizePhase()
{
	if(GEngine)
		GEngine->AddOnScreenDebugMessage(-1, 0.0f, FColor::Yellow, FString::Printf(TEXT("Phase is %f"), Phase));
}
