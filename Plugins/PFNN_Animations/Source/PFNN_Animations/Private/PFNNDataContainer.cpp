// Fill out your copyright notice in the Description page of Project Settings.


#include "PFNNDataContainer.h"
#include "PFNNHelperFunctions.h"

#ifdef DEFINE_UE_4_VERSION
#include "GenericPlatformFile.h"
#include "PlatformFilemanager.h"
#else
#include "GenericPlatform/GenericPlatformFile.h"
#include "HAL/PlatformFilemanager.h"
#endif // DEFINE_UE_4_VERSION

#include "Runtime/Core/Public/Misc/Paths.h"

UPFNNDataContainer::UPFNNDataContainer(const FObjectInitializer& arg_ObjectInitializer) 
	: Super(arg_ObjectInitializer)
	, bIsDataLoaded(false)
{
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

UPFNNDataContainer::~UPFNNDataContainer()
{
	UE_LOG(PFNN_Logging, Log, TEXT("PFNN Data Container is being deconstructed..."));
}

void UPFNNDataContainer::LoadNetworkData()
{
	DataLocker.Lock();

	if(IsDataLoaded())
	{
		UE_LOG(PFNN_Logging, Log, TEXT("Attempted to load PFNN data but it was already loaded. Attempt has been skipped."));
		SetIsBeingLoaded(false);
		DataLocker.Unlock();
		return;
	}

	UE_LOG(PFNN_Logging, Log, TEXT("Loading PFNN Data..."));

	Xmean = LoadWeights(XDIM, FString::Printf(TEXT("Plugins/PFNN_Animations/Content/MachineLearning/PhaseFunctionNeuralNetwork/Weights/Xmean.bin")));
	Xstd = LoadWeights(XDIM, FString::Printf(TEXT("Plugins/PFNN_Animations/Content/MachineLearning/PhaseFunctionNeuralNetwork/Weights/Xstd.bin")));
	Ymean = LoadWeights(YDIM, FString::Printf(TEXT("Plugins/PFNN_Animations/Content/MachineLearning/PhaseFunctionNeuralNetwork/Weights/Ymean.bin")));
	Ystd = LoadWeights(YDIM, FString::Printf(TEXT("Plugins/PFNN_Animations/Content/MachineLearning/PhaseFunctionNeuralNetwork/Weights/Ystd.bin")));

	int32 size_weights = 0;
	double index_scale = 1.0;
	const auto LoadingMode = GetDefault<UPhaseFunctionNeuralNetwork>()->Mode;
	switch(LoadingMode)
	{
	case EPFNNMode::PM_Constant:
		size_weights = 50;
		index_scale = 1.0;
		break;
	case EPFNNMode::PM_Linear:
		size_weights = 10;
		index_scale = 5.0;
		break;
	case EPFNNMode::PM_Cubic:
		size_weights = 4;
		index_scale = 12.5;
		break;
	}

	W0.SetNum(size_weights); W1.SetNum(size_weights); W2.SetNum(size_weights);
	b0.SetNum(size_weights); b1.SetNum(size_weights); b2.SetNum(size_weights);
	for(int i = 0; i < size_weights; i++)
	{
		const int32 index_scaled = fmod(i * index_scale, 1.0f);
		W0[i] = LoadWeights(HDIM, XDIM, FString::Printf(TEXT("Plugins/PFNN_Animations/Content/MachineLearning/PhaseFunctionNeuralNetwork/Weights/W0_%03d.bin"), index_scaled));
		W1[i] = LoadWeights(HDIM, HDIM, FString::Printf(TEXT("Plugins/PFNN_Animations/Content/MachineLearning/PhaseFunctionNeuralNetwork/Weights/W1_%03d.bin"), index_scaled));
		W2[i] = LoadWeights(YDIM, HDIM, FString::Printf(TEXT("Plugins/PFNN_Animations/Content/MachineLearning/PhaseFunctionNeuralNetwork/Weights/W2_%03d.bin"), index_scaled));
		b0[i] = LoadWeights(HDIM, FString::Printf(TEXT("Plugins/PFNN_Animations/Content/MachineLearning/PhaseFunctionNeuralNetwork/Weights/b0_%03d.bin"), index_scaled));
		b1[i] = LoadWeights(HDIM, FString::Printf(TEXT("Plugins/PFNN_Animations/Content/MachineLearning/PhaseFunctionNeuralNetwork/Weights/b1_%03d.bin"), index_scaled));
		b2[i] = LoadWeights(YDIM, FString::Printf(TEXT("Plugins/PFNN_Animations/Content/MachineLearning/PhaseFunctionNeuralNetwork/Weights/b2_%03d.bin"), index_scaled));
	}

	bIsDataLoaded = true;
	SetIsBeingLoaded(false);

	DataLocker.Unlock();

	UE_LOG(PFNN_Logging, Log, TEXT("Finished Loading PFNN Data"));
}

void UPFNNDataContainer::GetNetworkData(UPhaseFunctionNeuralNetwork& arg_PFNN)
{
	if(!arg_PFNN.W0.IsEmpty())
	{
		arg_PFNN.W0.Empty(); arg_PFNN.W1.Empty(); arg_PFNN.W2.Empty();
		arg_PFNN.b0.Empty(); arg_PFNN.b1.Empty(); arg_PFNN.b2.Empty();
	}

	arg_PFNN.Xmean = this->Xmean;
	arg_PFNN.Xstd = this->Xstd;
	arg_PFNN.Ymean = this->Ymean;
	arg_PFNN.Ystd = this->Ystd;

	arg_PFNN.W0 = this->W0; arg_PFNN.W1 = this->W1; arg_PFNN.W2 = this->W2;
	arg_PFNN.b0 = this->b0; arg_PFNN.b1 = this->b1; arg_PFNN.b2 = this->b2;

	arg_PFNN.Xp = this->Xp; arg_PFNN.Yp = this->Yp;
	arg_PFNN.H0 = this->H0; arg_PFNN.H1 = this->H1;
	arg_PFNN.W0p = this->W0p; arg_PFNN.W1p = this->W1p; arg_PFNN.W2p = this->W2p;
	arg_PFNN.b0p = this->b0p; arg_PFNN.b1p = this->b1p; arg_PFNN.b2p = this->b2p;
}

Eigen::ArrayXXf UPFNNDataContainer::LoadWeights(const int arg_Rows, const int arg_Cols, const FString arg_FileName, ...)
{
	Eigen::ArrayXXf arg_A(arg_Rows, arg_Cols);
	UPFNNHelperFileReader f(arg_FileName);
	if(!f.isOpen())
	{
		UE_LOG(PFNN_Logging, Error, TEXT("Fatal error, Failed to load Phase Function Neural Network weights. File name "));
	}
	else
	{
		for(int iRow = 0; iRow < arg_Rows; iRow++)
		{
			for(int iCol = 0; iCol < arg_Cols; iCol++)
				arg_A(iRow, iCol) = f.readItem().FloatValue;
		}
	}
	return arg_A;
}

Eigen::ArrayXf UPFNNDataContainer::LoadWeights(const int arg_Items, const FString arg_FileName, ...)
{
	Eigen::ArrayXf arg_V(arg_Items);
	UPFNNHelperFileReader f(arg_FileName);
	if(!f.isOpen())
	{
		UE_LOG(PFNN_Logging, Error, TEXT("Failed to load Weights file: %s"), *arg_FileName);
	}
	else
	{
		for(int32 iItem = 0; iItem < arg_Items; iItem++)
			arg_V(iItem) = f.readItem().FloatValue;
	}
	return arg_V;
}

bool UPFNNDataContainer::IsDataLoaded() const
{
	return bIsDataLoaded;
}

bool UPFNNDataContainer::IsBeingLoaded() const
{
	return bIsCurrentlyLoading;
}

void UPFNNDataContainer::SetIsBeingLoaded(bool arg_IsLoading)
{
	bIsCurrentlyLoading = arg_IsLoading;
}

FCriticalSection* UPFNNDataContainer::GetDataLocker()
{
	return &DataLocker;
}

FPFNNDataLoader::FPFNNDataLoader(UPFNNDataContainer* arg_PFNNDataContainer)
	: PFNNDataContainer(arg_PFNNDataContainer)
{}

FPFNNDataLoader::~FPFNNDataLoader()
{}

void FPFNNDataLoader::DoWork()
{
	PFNNDataContainer->LoadNetworkData();
}

PFNNWeigthLoader::PFNNWeigthLoader(WeigthLoadingMatrixDelegate arg_FunctionDelegate)
{
	MatrixDelegate = arg_FunctionDelegate;
}

PFNNWeigthLoader::PFNNWeigthLoader(WeigthLoadingVectorDelegate arg_FunctionDelegate)
{
	VectorDelegate = arg_FunctionDelegate;
}

PFNNWeigthLoader::~PFNNWeigthLoader()
{}

void PFNNWeigthLoader::DoWork()
{
	if(MatrixDelegate.IsBound())
		return;

	if(VectorDelegate.IsBound())
		return;

	UE_LOG(PFNN_Logging, Warning, TEXT("Attempting to Launch Weight loading thread without binding function delegates!"));
}
