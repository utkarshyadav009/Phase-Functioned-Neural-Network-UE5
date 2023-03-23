

#pragma once

#include "CoreMinimal.h"
#include "UObject/NoExportTypes.h"
#include "Engine/DeveloperSettings.h"
#include "ThirdParty/Eigen/Dense"
#include "PhaseFunctionNeuralNetwork.generated.h"

DECLARE_LOG_CATEGORY_EXTERN(PFNN_Logging, Log, All);

UENUM(BlueprintType)
enum class EPFNNMode : uint8 
{
	PM_Constant		UMETA(DisplayName="Constant"),
	PM_Linear		UMETA(DisplayName="Linear"),
	PM_Cubic		UMETA(DisplayName="Cubic")
};

/**
 * 
 */
UCLASS(Config=Engine, defaultconfig, meta=(DisplayName="PFNN Settings"))
class PFNN_API UPhaseFunctionNeuralNetwork : public UDeveloperSettings
{
public:
	GENERATED_BODY()
	UPhaseFunctionNeuralNetwork();
	~UPhaseFunctionNeuralNetwork();
	
	/*
	* @Description Load in the Phase Function Neural Network.
	*/
	bool LoadNetworkData(UObject* arg_ContextObject);

	/*
	* @Description Exponential Linear Unit(ELU) activation function
	* @Param[in] arg_X, Base of the Eigen array
	*/
	static void ELU(Eigen::ArrayXf& arg_X);

	/*
	* @Description Linear implementation of the PFNN more computation intensive but requires less memory
	* @Param[out] arg_Out, Calculation output
	* @Param[in] arg_Y0, Output eigen array
	* @Param[in] arg_Y1, Output eigen array
	* @Param[in] arg_MU, Bias
	*/
	static void Linear(Eigen::ArrayXf& arg_Out, const Eigen::ArrayXf& arg_Y0, const Eigen::ArrayXf& arg_Y1, float arg_MU);
	/*
	* @Description Linear implementation of the PFNN more computation intensive but requires less memory
	* @Param[out] arg_Out, Calculation output
	* @Param[in] arg_Y0, Output eigen array
	* @Param[in] arg_Y1, Output eigen array
	* @Param[in] arg_MU, Bias
	*/
	static void Linear(Eigen::ArrayXXf& arg_Out, const Eigen::ArrayXXf& arg_Y0, const Eigen::ArrayXXf& arg_Y1, float arg_MU);

	/*
	* @Description Cubic implementation of the PFNN more memory intensive but faster computation
	* @Param[out] arg_Out, Calculation output
	* @Param[in] arg_Y0, Output eigen array
	* @Param[in] arg_Y1, Output eigen array
	* @Param[in] arg_Y3, Output eigen array
	* @Param[in] arg_MU, Bias
	*/
	static void Cubic(Eigen::ArrayXf& arg_Out, const Eigen::ArrayXf& arg_Y0, const Eigen::ArrayXf& arg_Y1, const Eigen::ArrayXf& arg_Y2, const Eigen::ArrayXf& arg_Y3, float arg_MU);
	/*
	* @Description Linear implementation of the PFNN more computation intensive but requires less memory
	* @Param[out] arg_Out, Calculation output
	* @Param[in] arg_Y0, Output eigen array
	* @Param[in] arg_Y1, Output eigen array
	* @Param[in] arg_Y3, Output eigen array
	* @Param[in] arg_MU, Bias
	*/
	static void Cubic(Eigen::ArrayXXf& arg_Out, const Eigen::ArrayXXf& arg_Y0, const Eigen::ArrayXXf& arg_Y1, const Eigen::ArrayXXf& arg_Y2, const Eigen::ArrayXXf& arg_Y3, float arg_MU);

	/*
	* @Description Makes the neural network predict based on the given phase
	* @Param[in] arg_Phase, Phase to base the prediction upon
	*/
	void Predict(float arg_Phase);

public:

	UPROPERTY(EditAnywhere, Config, Category = "Network", meta = (ConfigRestartRequired = true))
	EPFNNMode Mode;

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
};
