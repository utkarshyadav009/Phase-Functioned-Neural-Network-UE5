

#include "Animation/PFNN/PFNNDataContainer.h"

#include "GenericPlatform/GenericPlatformFile.h"
#include "Runtime/Core/Public/Misc/Paths.h"
#include "HAL/PlatformFileManager.h"

UPFNNDataContainer::UPFNNDataContainer(const FObjectInitializer& arg_ObjectInitializer) : Super(arg_ObjectInitializer), bIsDataLoaded(false) 
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

void UPFNNDataContainer::LoadNetworkData(EPFNNMode arg_Mode)
{
	DataLocker.Lock();

	if (bIsDataLoaded)
	{
		UE_LOG(PFNN_Logging, Log, TEXT("Attempted to load PFNN data but it was already loaded. Attempt has been skipped."));
		bIsCurrentlyLoading = false;
		DataLocker.Unlock();
		return;
	}

	UE_LOG(PFNN_Logging, Log, TEXT("Loading PFNN Data..."));

	LoadWeights(Xmean, XDIM, FString::Printf(TEXT("Plugins/PFNN/Content/MachineLearning/PhaseFunctionNeuralNetwork/Weights/Xmean.bin")));
	LoadWeights(Xstd, XDIM, FString::Printf(TEXT("Plugins/PFNN/Content/MachineLearning/PhaseFunctionNeuralNetwork/Weights/Xstd.bin")));
	LoadWeights(Ymean, YDIM, FString::Printf(TEXT("Plugins/PFNN/Content/MachineLearning/PhaseFunctionNeuralNetwork/Weights/Ymean.bin")));
	LoadWeights(Ystd, YDIM, FString::Printf(TEXT("Plugins/PFNN/Content/MachineLearning/PhaseFunctionNeuralNetwork/Weights/Ystd.bin")));

	switch (arg_Mode)
	{
	case EPFNNMode::PM_Constant:

		W0.SetNum(50); W1.SetNum(50); W2.SetNum(50);
		b0.SetNum(50); b1.SetNum(50); b2.SetNum(50);

		for (int i = 0; i < 50; i++)
		{
			LoadWeights(W0[i], HDIM, XDIM, FString::Printf(TEXT("Plugins/PFNN/Content/MachineLearning/PhaseFunctionNeuralNetwork/Weights/W0_%03d.bin"), i));
			LoadWeights(W1[i], HDIM, HDIM, FString::Printf(TEXT("Plugins/PFNN/Content/MachineLearning/PhaseFunctionNeuralNetwork/Weights/W1_%03d.bin"), i));
			LoadWeights(W2[i], YDIM, HDIM, FString::Printf(TEXT("Plugins/PFNN/Content/MachineLearning/PhaseFunctionNeuralNetwork/Weights/W2_%03d.bin"), i));
			LoadWeights(b0[i], HDIM, FString::Printf(TEXT("Plugins/PFNN/Content/MachineLearning/PhaseFunctionNeuralNetwork/Weights/b0_%03d.bin"), i));
			LoadWeights(b1[i], HDIM, FString::Printf(TEXT("Plugins/PFNN/Content/MachineLearning/PhaseFunctionNeuralNetwork/Weights/b1_%03d.bin"), i));
			LoadWeights(b2[i], YDIM, FString::Printf(TEXT("Plugins/PFNN/Content/MachineLearning/PhaseFunctionNeuralNetwork/Weights/b2_%03d.bin"), i));
		}

		break;

	case EPFNNMode::PM_Linear:

		W0.SetNum(10); W1.SetNum(10); W2.SetNum(10);
		b0.SetNum(10); b1.SetNum(10); b2.SetNum(10);

		for (int i = 0; i < 10; i++)
		{
			LoadWeights(W0[i], HDIM, XDIM, FString::Printf(TEXT("Plugins/PFNN/Content/MachineLearning/PhaseFunctionNeuralNetwork/Weights/W0_%03d.bin"), i * 5));
			LoadWeights(W1[i], HDIM, HDIM, FString::Printf(TEXT("Plugins/PFNN/Content/MachineLearning/PhaseFunctionNeuralNetwork/Weights/W1_%03d.bin"), i * 5));
			LoadWeights(W2[i], YDIM, HDIM, FString::Printf(TEXT("Plugins/PFNN/Content/MachineLearning/PhaseFunctionNeuralNetwork/Weights/W2_%03d.bin"), i * 5));
			LoadWeights(b0[i], HDIM, FString::Printf(TEXT("Plugins/PFNN/Content/MachineLearning/PhaseFunctionNeuralNetwork/Weights/b0_%03d.bin"), i * 5));
			LoadWeights(b1[i], HDIM, FString::Printf(TEXT("Plugins/PFNN/Content/MachineLearning/PhaseFunctionNeuralNetwork/Weights/b1_%03d.bin"), i * 5));
			LoadWeights(b2[i], YDIM, FString::Printf(TEXT("Plugins/PFNN/Content/MachineLearning/PhaseFunctionNeuralNetwork/Weights/b2_%03d.bin"), i * 5));
		}

		break;

	case EPFNNMode::PM_Cubic:

		W0.SetNum(4); W1.SetNum(4); W2.SetNum(4);
		b0.SetNum(4); b1.SetNum(4); b2.SetNum(4);

		for (int i = 0; i < 4; i++)
		{
			LoadWeights(W0[i], HDIM, XDIM, FString::Printf(TEXT("Plugins/PFNN/Content/MachineLearning/PhaseFunctionNeuralNetwork/Weights/W0_%03i.bin"), static_cast<int>(i * 12.5)));
			LoadWeights(W1[i], HDIM, HDIM, FString::Printf(TEXT("Plugins/PFNN/Content/MachineLearning/PhaseFunctionNeuralNetwork/Weights/W1_%03i.bin"), static_cast<int>(i * 12.5)));
			LoadWeights(W2[i], YDIM, HDIM, FString::Printf(TEXT("Plugins/PFNN/Content/MachineLearning/PhaseFunctionNeuralNetwork/Weights/W2_%03i.bin"), static_cast<int>(i * 12.5)));
			LoadWeights(b0[i], HDIM, FString::Printf(TEXT("Plugins/PFNN/Content/MachineLearning/PhaseFunctionNeuralNetwork/Weights/b0_%03i.bin"), static_cast<int>(i * 12.5)));
			LoadWeights(b1[i], HDIM, FString::Printf(TEXT("Plugins/PFNN/Content/MachineLearning/PhaseFunctionNeuralNetwork/Weights/b1_%03i.bin"), static_cast<int>(i * 12.5)));
			LoadWeights(b2[i], YDIM, FString::Printf(TEXT("Plugins/PFNN/Content/MachineLearning/PhaseFunctionNeuralNetwork/Weights/b2_%03i.bin"), static_cast<int>(i * 12.5)));
		}

		break;
	}

	bIsDataLoaded = true;
	bIsCurrentlyLoading = false;

	DataLocker.Unlock();

	UE_LOG(PFNN_Logging, Log, TEXT("Finished Loading PFNN Data"));

}

void UPFNNDataContainer::GetNetworkData(UPhaseFunctionNeuralNetwork& arg_PFNN)
{
	arg_PFNN.Xmean	= this->Xmean;
	arg_PFNN.Xstd	= this->Xstd;
	arg_PFNN.Ymean	= this->Ymean;
	arg_PFNN.Ystd	= this->Ystd;

	arg_PFNN.W0 = this->W0; arg_PFNN.W1 = this->W1; arg_PFNN.W2 = this->W2;
	arg_PFNN.b0 = this->b0; arg_PFNN.b1 = this->b1; arg_PFNN.b2 = this->b2;

	arg_PFNN.Xp = this->Xp; arg_PFNN.Yp = this->Yp;
	arg_PFNN.H0 = this->H0; arg_PFNN.H1 = this->H1;
	arg_PFNN.W0p = this->W0p; arg_PFNN.W1p = this->W1p; arg_PFNN.W2p = this->W2p;
	arg_PFNN.b0p = this->b0p; arg_PFNN.b1p = this->b1p; arg_PFNN.b2p = this->b2p;
}

void UPFNNDataContainer::LoadWeights(Eigen::ArrayXXf& arg_A, const int arg_Rows, const int arg_Cols,
	const FString arg_FileName, ...)
{
	IPlatformFile& PlatformFile = FPlatformFileManager::Get().GetPlatformFile();

	FString RelativePath = FPaths::ProjectDir();
	const FString FullPath = RelativePath += arg_FileName;

	IFileHandle* FileHandle = PlatformFile.OpenRead(*FullPath);

	if (FileHandle == nullptr)
	{
		UE_LOG(PFNN_Logging, Error, TEXT("Fatal error, Failed to load Phase Function Neural Network weights. File name "));
		return;
	}

	arg_A = Eigen::ArrayXXf(arg_Rows, arg_Cols);
	for (int x = 0; x < arg_Rows; x++)
	{
		for (int y = 0; y < arg_Cols; y++)
		{
			FFloat32 item;
			uint8* ByteBuffer = reinterpret_cast<uint8*>(&item);

			FileHandle->Read(ByteBuffer, sizeof(FFloat32));
			arg_A(x, y) = item.FloatValue;
		}
	}

	delete FileHandle;

}

void UPFNNDataContainer::LoadWeights(Eigen::ArrayXf& arg_V, const int arg_Items, const FString arg_FileName, ...)
{

	IPlatformFile& PlatformFile = FPlatformFileManager::Get().GetPlatformFile();

	FString RelativePath = FPaths::ProjectDir();
	const FString FullPath = RelativePath += arg_FileName;

	IFileHandle* FileHandle = PlatformFile.OpenRead(*FullPath);

	if (FileHandle == nullptr)
	{
		UE_LOG(PFNN_Logging, Error, TEXT("Failed to load Weights file: %s"), *arg_FileName);
		return;
	}

	arg_V = Eigen::ArrayXf(arg_Items);

	for (int i = 0; i < arg_Items; i++)
	{
		FFloat32 item;
		uint8* ByteBuffer = reinterpret_cast<uint8*>(&item);

		FileHandle->Read(ByteBuffer, sizeof(FFloat32));
		arg_V(i) = item.FloatValue;
	}

	delete FileHandle;
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

PFNNDataLoader::PFNNDataLoader(UPFNNDataContainer* arg_PFNNDataContainer) : PFNNDataContainer(arg_PFNNDataContainer)
{
}

PFNNDataLoader::~PFNNDataLoader()
{
}

void PFNNDataLoader::DoWork()
{
	PFNNDataContainer->LoadNetworkData(EPFNNMode::PM_Cubic);
}

PFNNWeightLoader::PFNNWeightLoader(WeightLoadingMatrixDelegate arg_FunctionDelegate)
{
	MatrixDelegate = arg_FunctionDelegate;
}

PFNNWeightLoader::PFNNWeightLoader(WeightLoadingVectorDelegate arg_FunctionDelegate)
{
	VectorDelegate = arg_FunctionDelegate;
}

PFNNWeightLoader::~PFNNWeightLoader()
{
}

void PFNNWeightLoader::DoWork()
{
	if (MatrixDelegate.IsBound())
	{
		return;
	}
	if (VectorDelegate.IsBound())
	{
		return;
	}
	UE_LOG(PFNN_Logging, Warning, TEXT("Attempting to Launch Weight loading thread without binding function delegates!"));
}
