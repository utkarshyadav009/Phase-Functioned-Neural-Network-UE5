
#pragma once

#include "PhaseFunctionNeuralNetwork.h"

#include "CoreMinimal.h"
#include "UObject/NoExportTypes.h"

#include "Async/AsyncWork.h"

#include "PFNNDataContainer.generated.h"

DECLARE_DELEGATE_FourParams(WeightLoadingMatrixDelegate, Eigen::ArrayXXf&, const int, const int, FString)
DECLARE_DELEGATE_ThreeParams(WeightLoadingVectorDelegate, Eigen::ArrayXXf&, const int, FString)

class UPhaseFunctionNeuralNetwork;

/**
 * 
 */
UCLASS()
class PFNN_API UPFNNDataContainer : public UObject
{
	GENERATED_BODY()

public:

	UPFNNDataContainer(const FObjectInitializer& arg_ObjectInitializer);

	~UPFNNDataContainer();

	/*
	* @Description Load in the Phase Function Neural Network.
	*/
	void LoadNetworkData(EPFNNMode arg_Mode);
	/*
	* @Description Puts data into the passed PFNN object
	* @Param[out] arg_PFNN, The object that will recieve the data
	*/
	void GetNetworkData(UPhaseFunctionNeuralNetwork& arg_PFNN);
	
	bool IsDataLoaded() const;

	bool IsBeingLoaded() const;

	void SetIsBeingLoaded(bool arg_IsLoaded);

	FCriticalSection* GetDataLocker();

private:

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, meta = (AllowPrivateAccess = "True"))
	bool bIsDataLoaded;

	UPROPERTY()
	bool bIsCurrentlyLoading;

	EPFNNMode LoadingMode;

	/*
	* @Description Load weights for the Phase Function Neural Network
	* @Param[in] arg_A, Base of eigen array
	* @Param[in] arg_Rows, Number of rows in the matrix
	* @Param[in] arg_Cols, Number of colums in the matrix
	* @Param[in] arg_FileName, The file path where to find the Neural Network data
	*/
	void LoadWeights(Eigen::ArrayXXf& arg_A, const int arg_Rows, const int arg_Cols, const FString arg_FileName, ...);
	/*
	* @Description Load weights for the Phase Function Neural Network
	* @Param[in] arg_V, Base of eigen array
	* @Param[in] arg_Items, Items that need to be loaded in
	* @Param[in] arg_FileName, The file path where to find the Neural Network data
	*/
	void LoadWeights(Eigen::ArrayXf &arg_V, const int arg_Items, const FString arg_FileName, ...);

	//DO NOT CHANGE! These values determine the dimentions of the Neural Network. Changing them without knowing what you are doing will crash the engine.
	enum { XDIM = 342, YDIM = 311, HDIM = 512 };

	Eigen::ArrayXf Xmean, Xstd;
	Eigen::ArrayXf Ymean, Ystd;

	TArray<Eigen::ArrayXXf> W0, W1, W2;
	TArray<Eigen::ArrayXf>  b0, b1, b2;

	Eigen::ArrayXf  Xp, Yp;
	Eigen::ArrayXf  H0, H1;
	Eigen::ArrayXXf W0p, W1p, W2p;
	Eigen::ArrayXf  b0p, b1p, b2p;
	//Ending of things that you should not change.

	//Mutex for ensuring ThreadSafety of the DataContainer
	FCriticalSection DataLocker;

};

class PFNNDataLoader : public FNonAbandonableTask
{

public:

	PFNNDataLoader(UPFNNDataContainer* arg_PFNNDataContainer);

	~PFNNDataLoader();

	FORCEINLINE TStatId GetStatId() const
	{
		RETURN_QUICK_DECLARE_CYCLE_STAT(PFNNDataLoader, STATGROUP_ThreadPoolAsyncTasks)
	}

	void DoWork();

private:

	UPROPERTY()
	UPFNNDataContainer* PFNNDataContainer;

};

class PFNNWeightLoader : public FNonAbandonableTask
{
public:

	PFNNWeightLoader(WeightLoadingMatrixDelegate arg_FunctionDelegate);
	PFNNWeightLoader(WeightLoadingVectorDelegate arg_FunctionDelegate);

	~PFNNWeightLoader();

	FORCEINLINE TStatId GetStatId() const
	{
		RETURN_QUICK_DECLARE_CYCLE_STAT(PFNNWeightLoader, STATGROUP_ThreadPoolAsyncTasks)
	}

	void DoWork();

private:

	WeightLoadingMatrixDelegate MatrixDelegate;
	WeightLoadingVectorDelegate VectorDelegate;

};