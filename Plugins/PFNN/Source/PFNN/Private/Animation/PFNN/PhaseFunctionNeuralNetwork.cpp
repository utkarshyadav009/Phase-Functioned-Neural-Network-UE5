// Copyright 2018 Sticks & Stones. All Rights Reserved.

#include "Animation/PFNN/PhaseFunctionNeuralNetwork.h"

#include "Engine/Engine.h"
#include "Kismet/GameplayStatics.h"
#include "GenericPlatform/GenericPlatformFile.h"
#include "Animation/PFNN/PFNNDataContainer.h"
#include "Core/GameInstance/PFNNGameInstance.h"

DEFINE_LOG_CATEGORY(PFNN_Logging);

UPhaseFunctionNeuralNetwork::UPhaseFunctionNeuralNetwork() : Mode(EPFNNMode::PM_Cubic)
{
	UE_LOG(PFNN_Logging, Log, TEXT("Creating PhaseFunctionNeuralNetwork Object"));

	Xp = Eigen::ArrayXf(static_cast<int>(XDIM));
	Yp = Eigen::ArrayXf(static_cast<int>(YDIM));

	H0 = Eigen::ArrayXf(static_cast<int>(HDIM));
	H1 = Eigen::ArrayXf(static_cast<int>(HDIM));

	W0p = Eigen::ArrayXXf(static_cast<int>(HDIM), static_cast<int>(XDIM));
	W1p = Eigen::ArrayXXf(static_cast<int>(HDIM), static_cast<int>(HDIM));
	W2p = Eigen::ArrayXXf(static_cast<int>(YDIM), static_cast<int>(HDIM));

	b0p = Eigen::ArrayXf(static_cast<int>(HDIM));
	b1p = Eigen::ArrayXf(static_cast<int>(HDIM));
	b2p = Eigen::ArrayXf(static_cast<int>(YDIM));

}

UPhaseFunctionNeuralNetwork::~UPhaseFunctionNeuralNetwork()
{
}

bool UPhaseFunctionNeuralNetwork::LoadNetworkData(UObject* arg_ContextObject)
{
	UPFNNGameInstance* GameInstance = Cast<UPFNNGameInstance>(UGameplayStatics::GetGameInstance(arg_ContextObject));
	if (GameInstance) 
	{
		UPFNNDataContainer* DataContainer = GameInstance->GetPFNNDataContainer();
		if (DataContainer->GetDataLocker()->TryLock()) //Successfull Lock means we aren't currently writing to the DataContainer and we can safely lock
		{
			if (!DataContainer->IsBeingLoaded())
			{
				if (DataContainer->IsDataLoaded())
				{
					DataContainer->GetNetworkData(*this);
					DataContainer->GetDataLocker()->Unlock();
					return true;
				}
				DataContainer->SetIsBeingLoaded(true);
				GameInstance->LoadPFNNDataAsync();
			}
			else
			{
				GEngine->AddOnScreenDebugMessage(-1, 0.0f, FColor::Green, TEXT("PFNN Files are being loaded..."));
			}
			DataContainer->GetDataLocker()->Unlock();
		}
	}
	else 
	{
		GEngine->AddOnScreenDebugMessage(-1, 1.0f, FColor::Green, TEXT("Invalid Shitty Code..."));

		UE_LOG(PFNN_Logging, Error, TEXT("Invalid GameInstance for PFNN"));
	}
	return false;
}

void UPhaseFunctionNeuralNetwork::ELU(Eigen::ArrayXf& arg_X)
{
	arg_X = arg_X.max(0) + arg_X.min(0).exp() - 1;
}

void UPhaseFunctionNeuralNetwork::Linear(Eigen::ArrayXf& arg_Out, const Eigen::ArrayXf& arg_Y0, const Eigen::ArrayXf& arg_Y1, float arg_MU)
{
	arg_Out = (1.0f - arg_MU) * arg_Y0 + (arg_MU)* arg_Y1;
}
void UPhaseFunctionNeuralNetwork::Linear(Eigen::ArrayXXf& arg_Out, const Eigen::ArrayXXf& arg_Y0, const Eigen::ArrayXXf& arg_Y1, float arg_MU)
{
	arg_Out = (1.0f - arg_MU) * arg_Y0 + (arg_MU)* arg_Y1;
}

void UPhaseFunctionNeuralNetwork::Cubic(Eigen::ArrayXf& arg_Out, const Eigen::ArrayXf& arg_Y0, const Eigen::ArrayXf& arg_Y1, const Eigen::ArrayXf& arg_Y2, const Eigen::ArrayXf& arg_Y3, float arg_MU)
{
	arg_Out = (
		(-0.5 * arg_Y0 + 1.5 * arg_Y1 - 1.5 * arg_Y2 + 0.5 * arg_Y3) * arg_MU * arg_MU * arg_MU +
		(arg_Y0 - 2.5 * arg_Y1 + 2.0 * arg_Y2 - 0.5 * arg_Y3) * arg_MU * arg_MU +
		(-0.5 * arg_Y0 + 0.5 * arg_Y2) * arg_MU +
		(arg_Y1));
}
void UPhaseFunctionNeuralNetwork::Cubic(Eigen::ArrayXXf& arg_Out, const Eigen::ArrayXXf& arg_Y0, const Eigen::ArrayXXf& arg_Y1, const Eigen::ArrayXXf& arg_Y2, const Eigen::ArrayXXf& arg_Y3, float arg_MU)
{
	arg_Out = (
		(-0.5 * arg_Y0 + 1.5 * arg_Y1 - 1.5 * arg_Y2 + 0.5 * arg_Y3) * arg_MU * arg_MU * arg_MU +
		(arg_Y0 - 2.5 * arg_Y1 + 2.0 * arg_Y2 - 0.5 * arg_Y3) * arg_MU * arg_MU +
		(-0.5 * arg_Y0 + 0.5 * arg_Y2) * arg_MU +
		(arg_Y1));
}

void UPhaseFunctionNeuralNetwork::Predict(float arg_Phase)
{
	float pamount;
	int pindex_0;
	int pindex_1;
	int pindex_2;
	int pindex_3;

	Xp = (Xp - Xmean) / Xstd;

	arg_Phase = FMath::Abs(arg_Phase);
	switch (Mode)
	{
	case EPFNNMode::PM_Constant:
		pindex_1 = static_cast<int>((arg_Phase / (2 * PI)) * 50);
		H0 = (W0[pindex_1].matrix() * Xp.matrix()).array() + b0[pindex_1];
		ELU(H0);
		H1 = (W1[pindex_1].matrix() * H0.matrix()).array() + b1[pindex_1];
		ELU(H1);
		Yp = (W2[pindex_1].matrix() * H1.matrix()).array() + b2[pindex_1];
		break;

	case EPFNNMode::PM_Linear:
		pamount = fmod((arg_Phase / (2 * PI)) * 10, 1.0);
		pindex_1 = (int)((arg_Phase / (2 * PI)) * 10);
		pindex_2 = ((pindex_1 + 1) % 10);
		Linear(W0p, W0[pindex_1], W0[pindex_2], pamount);
		Linear(W1p, W1[pindex_1], W1[pindex_2], pamount);
		Linear(W2p, W2[pindex_1], W2[pindex_2], pamount);
		Linear(b0p, b0[pindex_1], b0[pindex_2], pamount);
		Linear(b1p, b1[pindex_1], b1[pindex_2], pamount);
		Linear(b2p, b2[pindex_1], b2[pindex_2], pamount);
		H0 = (W0p.matrix() * Xp.matrix()).array() + b0p;
		ELU(H0);
		H1 = (W1p.matrix() * H0.matrix()).array() + b1p;
		ELU(H1);
		Yp = (W2p.matrix() * H1.matrix()).array() + b2p;
		break;

	case EPFNNMode::PM_Cubic:
		pamount = fmod((arg_Phase / (2 * PI)) * 4, 1.0);
		pindex_1 = (int)((arg_Phase / (2 * PI)) * 4);
		pindex_0 = ((pindex_1 + 3) % 4);
		pindex_2 = ((pindex_1 + 1) % 4);
		pindex_3 = ((pindex_1 + 2) % 4);
		Cubic(W0p, W0[pindex_0], W0[pindex_1], W0[pindex_2], W0[pindex_3], pamount);
		Cubic(W1p, W1[pindex_0], W1[pindex_1], W1[pindex_2], W1[pindex_3], pamount);
		Cubic(W2p, W2[pindex_0], W2[pindex_1], W2[pindex_2], W2[pindex_3], pamount);
		Cubic(b0p, b0[pindex_0], b0[pindex_1], b0[pindex_2], b0[pindex_3], pamount);
		Cubic(b1p, b1[pindex_0], b1[pindex_1], b1[pindex_2], b1[pindex_3], pamount);
		Cubic(b2p, b2[pindex_0], b2[pindex_1], b2[pindex_2], b2[pindex_3], pamount);
		H0 = (W0p.matrix() * Xp.matrix()).array() + b0p;
		ELU(H0);
		H1 = (W1p.matrix() * H0.matrix()).array() + b1p;
		ELU(H1);
		Yp = (W2p.matrix() * H1.matrix()).array() + b2p;
		break;

	default:
		break;
	}

	Yp = (Yp * Ystd) + Ymean;
}
