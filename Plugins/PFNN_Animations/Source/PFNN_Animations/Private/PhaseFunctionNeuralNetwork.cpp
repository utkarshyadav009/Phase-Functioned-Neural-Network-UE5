// Fill out your copyright notice in the Description page of Project Settings.
#include "PhaseFunctionNeuralNetwork.h"

#include "Engine/Engine.h"
#include "Kismet/GameplayStatics.h"
#ifdef DEFINE_UE_4_VERSION
#include "GenericPlatformFile.h"
#else
#include "GenericPlatform/GenericPlatformFile.h"
#endif // DEFINE_UE_4_VERSION

#include "PFNNDataContainer.h"
#include "PFNNGameInstance.h"

DEFINE_LOG_CATEGORY(PFNN_Logging);

UPhaseFunctionNeuralNetwork::UPhaseFunctionNeuralNetwork()
	: Mode(EPFNNMode::PM_Cubic)
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
{}

bool UPhaseFunctionNeuralNetwork::LoadNetworkData(UObject* arg_ContextObject)
{
	UPFNNGameInstance* GameInstance = Cast<UPFNNGameInstance>(UGameplayStatics::GetGameInstance(arg_ContextObject));
	if(!GameInstance)
	{
		UE_LOG(PFNN_Logging, Error, TEXT("Invalid GameInstance for PFNN"));
		return false;
	}

	UPFNNDataContainer* DataContainer = GameInstance->GetPFNNDataContainer();
	if(DataContainer->GetDataLocker()->TryLock()) //Successfull Lock means we aren't currently writing to the DataContainer and we can safely lock
	{
		if(!DataContainer->IsBeingLoaded())
		{
			if(!DataContainer->IsDataLoaded())
			{
				DataContainer->SetIsBeingLoaded(true);
				GameInstance->LoadPFNNDataAsync();
			}
			else
			{
				DataContainer->GetNetworkData(*this);
				DataContainer->GetDataLocker()->Unlock();
				return true;
			}
		}
		else
		{
			GEngine->AddOnScreenDebugMessage(-1, 0.0f, FColor::Green, TEXT("PFNN Files are being loaded..."));
		}
		DataContainer->GetDataLocker()->Unlock();
	}
	return false;
}

void UPhaseFunctionNeuralNetwork::ELU(Eigen::ArrayXf& arg_X)
{
	arg_X = arg_X.max(0) + arg_X.min(0).exp() - 1;
}

Eigen::ArrayXf UPhaseFunctionNeuralNetwork::Linear(const Eigen::ArrayXf& arg_Y0, const Eigen::ArrayXf& arg_Y1, float arg_MU)
{
	return (1.0f - arg_MU) * arg_Y0 + arg_MU * arg_Y1;
}
Eigen::ArrayXXf UPhaseFunctionNeuralNetwork::Linear(const Eigen::ArrayXXf& arg_Y0, const Eigen::ArrayXXf& arg_Y1, float arg_MU)
{
	return (1.0f - arg_MU) * arg_Y0 + arg_MU * arg_Y1;
}

Eigen::ArrayXf UPhaseFunctionNeuralNetwork::Cubic(const Eigen::ArrayXf& arg_Y0, const Eigen::ArrayXf& arg_Y1, const Eigen::ArrayXf& arg_Y2, const Eigen::ArrayXf& arg_Y3, float arg_MU)
{
	return (-0.5 * arg_Y0 + 1.5 * arg_Y1 - 1.5 * arg_Y2 + 0.5 * arg_Y3) * powf(arg_MU, 3) +
		(arg_Y0 - 2.5 * arg_Y1 + 2.0 * arg_Y2 - 0.5 * arg_Y3) * powf(arg_MU, 2) +
		(-0.5 * arg_Y0 + 0.5 * arg_Y2) * arg_MU +
		arg_Y1;
}
Eigen::ArrayXXf UPhaseFunctionNeuralNetwork::Cubic(const Eigen::ArrayXXf& arg_Y0, const Eigen::ArrayXXf& arg_Y1, const Eigen::ArrayXXf& arg_Y2, const Eigen::ArrayXXf& arg_Y3, float arg_MU)
{
	return (-0.5 * arg_Y0 + 1.5 * arg_Y1 - 1.5 * arg_Y2 + 0.5 * arg_Y3) * powf(arg_MU, 3) +
		(arg_Y0 - 2.5 * arg_Y1 + 2.0 * arg_Y2 - 0.5 * arg_Y3) * powf(arg_MU, 2) +
		(-0.5 * arg_Y0 + 0.5 * arg_Y2) * arg_MU +
		arg_Y1;
}

void UPhaseFunctionNeuralNetwork::Predict(float arg_Phase)
{
	float pamount = 0.f;
	int32 pindex_0 = 0;
	int32 pindex_1 = 0;
	int32 pindex_2 = 0;
	int32 pindex_3 = 0;

	Xp = (Xp - Xmean) / Xstd;
	const SIZE_T weight_size = W0.Num();

	constexpr float cPI = 2 * PI;
	ensure(arg_Phase >= 0 && arg_Phase <= cPI); // check updated Phase 
	arg_Phase = FMath::Abs(arg_Phase);

	pamount = fmod((arg_Phase / cPI) * weight_size, 1.0);
	pindex_0 = fmod((arg_Phase / cPI) * weight_size, 1.0f);

	switch(Mode)
	{
	case EPFNNMode::PM_Constant:
		ensure(weight_size == 50);
		UE_LOG(PFNN_Logging
			   , Log
			   , TEXT("PFNN_Predict: %f weight_size, %d pindex_0")
			   , weight_size, pindex_0);

		W0p = W0[pindex_0]; b0p = b0[pindex_0];
		W1p = W1[pindex_0]; b1p = b1[pindex_0];
		W2p = W2[pindex_0]; b2p = b2[pindex_0];
		break;

	case EPFNNMode::PM_Linear:
		ensure(weight_size == 10);
		pindex_1 = ((pindex_0 + 1) % weight_size);

		UE_LOG(PFNN_Logging
			   , Log
			   , TEXT("PFNN_Predict: %f weight_size, %d pamount, %d pindex_3, %d pindex_0, %d pindex_1")
			   , weight_size, pamount, pindex_3, pindex_0, pindex_1);

		W0p = Linear(W0[pindex_0], W0[pindex_1], pamount); b0p = Linear(b0[pindex_0], b0[pindex_1], pamount);
		W1p = Linear(W1[pindex_0], W1[pindex_1], pamount); b1p = Linear(b1[pindex_0], b1[pindex_1], pamount);
		W2p = Linear(W2[pindex_0], W2[pindex_1], pamount); b2p = Linear(b2[pindex_0], b2[pindex_1], pamount);
		break;

	case EPFNNMode::PM_Cubic:
		ensure(weight_size == 4);
		pindex_3 = ((pindex_0 + 3) % weight_size);
		pindex_1 = ((pindex_0 + 1) % weight_size);
		pindex_2 = ((pindex_0 + 2) % weight_size);

		UE_LOG(PFNN_Logging
			   , Log
			   , TEXT("PFNN_Predict: %f weight_size, %d pamount, %d pindex_3, %d pindex_0, %d pindex_1, %d pindex_2")
			   , weight_size, pamount, pindex_3, pindex_0, pindex_1, pindex_2);

		W0p = Cubic(W0[pindex_3], W0[pindex_0], W0[pindex_1], W0[pindex_2], pamount); b0p = Cubic(b0[pindex_3], b0[pindex_0], b0[pindex_1], b0[pindex_2], pamount);
		W1p = Cubic(W1[pindex_3], W1[pindex_0], W1[pindex_1], W1[pindex_2], pamount); b1p = Cubic(b1[pindex_3], b1[pindex_0], b1[pindex_1], b1[pindex_2], pamount);
		W2p = Cubic(W2[pindex_3], W2[pindex_0], W2[pindex_1], W2[pindex_2], pamount); b2p = Cubic(b2[pindex_3], b2[pindex_0], b2[pindex_1], b2[pindex_2], pamount);
		break;

	default:
		break;
	}

	H0 = (W0p.matrix() * Xp.matrix()).array() + b0p; ELU(H0);
	H1 = (W1p.matrix() * H0.matrix()).array() + b1p; ELU(H1);
	Yp = (W2p.matrix() * H1.matrix()).array() + b2p;

	Yp = (Yp * Ystd) + Ymean;
}
