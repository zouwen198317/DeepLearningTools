#include "opencv2/opencv.hpp"
#include "opencv2/ml/ml.hpp"
#include <stdio.h>
#include <fstream>
using namespace std;
using namespace cv;

#define TRAINING_SAMPLES 496    //訓練データの数
#define ATTRIBUTES 64           //入力ベクトルの要素数
#define TEST_SAMPLES 496        //テストデータの数
#define CLASSES 10               //ラベルの種類,ラブライブ！主要メンバー9+その他=10

/* csvを読み込む関数
 * 各行が一つのデータに対応
 * 最初のATTRIBUTES列がデータ、最後の列がラベル */
void read_dataset(char *filename, Mat &data, Mat &classes,  int total_samples)
{
    int label;
    float pixelvalue;
    FILE* inputfile = fopen( filename, "r" );

    for(int row = 0; row < total_samples; row++){
        for(int col = 0; col <=ATTRIBUTES; col++){
            if (col < ATTRIBUTES){
                fscanf(inputfile, "%f,", &pixelvalue);
                data.at<float>(row,col) = pixelvalue;
            }
            else if (col == ATTRIBUTES){
                fscanf(inputfile, "%i", &label);
                classes.at<float>(row,label) = 1.0;
            }
        }
    }
    fclose(inputfile);
}

int main( int argc, char** argv ) {
    //訓練データを入れる行列
    Mat training_set(TRAINING_SAMPLES,ATTRIBUTES,CV_32F);
    //訓練データのラベルを入れる行列
    Mat training_set_classifications(TRAINING_SAMPLES, CLASSES, CV_32F);
    //テストデータを入れる行列
    Mat test_set(TEST_SAMPLES,ATTRIBUTES,CV_32F);
    //テストラベルを入れる行列
    Mat test_set_classifications(TEST_SAMPLES,CLASSES,CV_32F);

    //分類結果を入れる行列
    Mat classificationResult(1, CLASSES, CV_32F);
    //訓練データとテストデータのロード
    read_dataset(argv[1], training_set, training_set_classifications, TRAINING_SAMPLES);
    read_dataset(argv[2], test_set, test_set_classifications, TEST_SAMPLES);

    // ニューラルネットワークの定義
    Mat layers(3,1,CV_32S);          //三層構造
    layers.at<int>(0,0) = ATTRIBUTES;    //入力レイヤーの数
    layers.at<int>(1,0)=16;              //隠れユニットの数
    layers.at<int>(2,0) =CLASSES;        //出力レイヤーの数

    //ニューラルネットワークの構築
    CvANN_MLP nnetwork(layers, CvANN_MLP::SIGMOID_SYM,0.6,1);

    CvANN_MLP_TrainParams params(                                  
            // 一定回数繰り返すか変化が小さくなったら終了
            cvTermCriteria(CV_TERMCRIT_ITER+CV_TERMCRIT_EPS, 1000, 0.000001),
            // 訓練方法の指定。誤差逆伝播を使用
            CvANN_MLP_TrainParams::BACKPROP, 0.1, 0.1);

    // 訓練
    printf("\nUsing training dataset\n");
    int iterations = nnetwork.train(training_set, training_set_classifications, Mat(), Mat(),params);
    printf( "Training iterations: %i\n\n", iterations);

    // 訓練結果をxmlとして保存
    CvFileStorage* storage = cvOpenFileStorage("param.xml", 0, CV_STORAGE_WRITE );
    nnetwork.write(storage,"DigitOCR");
    cvReleaseFileStorage(&storage);

    // テストデータで訓練結果を確認
    Mat test_sample;
    int correct_class = 0;
    int wrong_class = 0;

    //分類結果を入れる配列
    int classification_matrix[CLASSES][CLASSES]={{}};

    for (int tsample = 0; tsample < TEST_SAMPLES; tsample++) {
        test_sample = test_set.row(tsample);
        nnetwork.predict(test_sample, classificationResult);
        // 最大の重みを持つクラスに分類
        int maxIndex = 0;
        float value=0.0f;
        float maxValue=classificationResult.at<float>(0,0);
        for(int index=1;index<CLASSES;index++){
            value = classificationResult.at<float>(0,index);
            if(value>maxValue){
                maxValue = value;
                maxIndex=index;
            }
        }

        //正解との比較
        if (test_set_classifications.at<float>(tsample, maxIndex)!=1.0f){
            cout << tsample << endl;
            wrong_class++;
            //find the actual label 'class_index'
            for(int class_index=0;class_index<CLASSES;class_index++) {
                if(test_set_classifications.at<float>(tsample, class_index)==1.0f){
                    classification_matrix[class_index][maxIndex]++;// A class_index sample was wrongly classified as maxindex.
                    break;
                }
            }
        } else {
            correct_class++;
            classification_matrix[maxIndex][maxIndex]++;
        }
    }

    printf( "\nResults on the testing dataset\n"
            "\tCorrect classification: %d (%g%%)\n"
            "\tWrong classifications: %d (%g%%)\n", 
            correct_class, (double) correct_class*100/TEST_SAMPLES,
            wrong_class, (double) wrong_class*100/TEST_SAMPLES);
    cout<<"   ";
    for (int i = 0; i < CLASSES; i++) cout<< i<< "\t";
    cout<<"\n";
    for(int row=0;row<CLASSES;row++){
        cout<<row<<"  ";
        for(int col=0;col<CLASSES;col++){
            cout << classification_matrix[row][col] << "\t";
        }
        cout<<"\n";
    }
    return 0;
}
