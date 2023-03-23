
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
	/*
	XDIM: The number of input neurons in the PFNN.
	YDIM: The number of output neurons in the PFNN.
	HDIM: The number of hidden neurons in the PFNN.
	*/
	enum { XDIM = 342, YDIM = 311, HDIM = 512 };

	/*
	Xmean: The mean of the input data used for normalization during training of the PFNN.
	Xstd: The standard deviation of the input data used for normalization during training of the PFNN.
	Ymean: The mean of the output data used for normalization during training of the PFNN.
	Ystd: The standard deviation of the output data used for normalization during training of the PFNN.
	*/
	Eigen::ArrayXf Xmean;
	Eigen::ArrayXf Xstd;
	Eigen::ArrayXf Ymean;
	Eigen::ArrayXf Ystd;

	/**/

	// The weight matrices of the PFNN between the input layer and the first hidden layer, between the first hidden layer and the second hidden layer, and between the second hidden layer and the output layer, respectively.
	TArray<Eigen::ArrayXXf> W0;
	TArray<Eigen::ArrayXXf> W1;
	TArray<Eigen::ArrayXXf> W2;

	// The bias vectors of the PFNN for the first hidden layer, second hidden layer, and output layer, respectively.
	TArray<Eigen::ArrayXf> b0;
	TArray<Eigen::ArrayXf> b1;
	TArray<Eigen::ArrayXf> b2;

	// The input data for a single frame of the animation.
	Eigen::ArrayXf Xp;
	// The output data for a single frame of the animation.
	Eigen::ArrayXf Yp;
	// The hidden layer activations for the first and second hidden layers, respectively.
	Eigen::ArrayXf H0;
	Eigen::ArrayXf H1;

	/*
	*W0p, W1p, and W2p: The weight matrices for the prediction part of the PFNN 
	 between the input layer and the first hidden layer, between the first hidden 
	 layer and the second hidden layer, and between the second hidden layer and 
	 the output layer, respectively.
	*/
	Eigen::ArrayXXf W0p;
	Eigen::ArrayXXf W1p;
	Eigen::ArrayXXf W2p;

	/*
	 b0p, b1p, and b2p: The bias vectors for the prediction part of 
	 the PFNN for the first hidden layer, second hidden layer, and output layer, respectively.
	*/
	Eigen::ArrayXf b0p;
	Eigen::ArrayXf b1p;
	Eigen::ArrayXf b2p;

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