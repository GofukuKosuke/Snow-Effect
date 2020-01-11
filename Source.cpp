#include <opencv2/opencv.hpp>
#pragma comment(lib, "opencv_world341.lib")
using namespace std;
using namespace cv;

// マクロ
#define FPS 30

// プロトタイプ宣言
int randInt(int min, int max);

// 雪構造体
typedef struct {
	Point pos;
} Snow;

int main() {

	// 変数等の宣言
	Mat src, diff, srcFloat, dstFloat, diffFloat, binary, edge, edge2;
	int t_mask = 20, t_canny1 = 20, t_canny2 = 35; // 閾値
	int erosion = 2; // 収縮値
	int currentFrame = 0; // 累計フレーム数
	vector<Snow> snows;

	// カメラの読み込み
	VideoCapture cap(0);
	Size cameraSize( (int)cap.get(CAP_PROP_FRAME_WIDTH), (int)cap.get(CAP_PROP_FRAME_HEIGHT) );

	VideoWriter rec("video.avi", VideoWriter::fourcc('X', 'V', 'I', 'D'), FPS / 2, cameraSize, true);

	// ウィンドウとトラックバーの生成
	namedWindow("差分マスク");
	createTrackbar("Threshold", "差分マスク", &t_mask, 255);
	createTrackbar("Erosion", "差分マスク", &erosion, 5);

	namedWindow("エッジ");
	createTrackbar("Threshold1", "エッジ", &t_canny1, 255);
	createTrackbar("Threshold2", "エッジ", &t_canny2, 255);

	dstFloat.create(cameraSize, CV_32FC3); // 小数型領域確保
	dstFloat.setTo(0.0); // 0.0で初期化

	while (1) {

		currentFrame++;

		// カメラ入力
		cap >> src;
		if (src.empty()) {
			waitKey(100);
			continue;
		}
		flip(src, src, 1); // 左右反転

		// エッジ検出
		GaussianBlur(src, edge, Size(5, 5), 2, 2);
		Canny(edge, edge2, t_canny1, t_canny2);
		dilate(edge2, edge2, cv::Mat(), Point(-1, -1), 1);
		imshow("エッジ", edge2);
		
		// 背景映像の初期化
		if (currentFrame == 1) {
			src.convertTo(dstFloat, CV_32FC3, 1 / 255.0);
		}

		// 背景映像
		src.convertTo(srcFloat, CV_32FC3, 1 / 255.0); // 小数型に変換
		addWeighted(srcFloat, 0.03, dstFloat, 0.97, 0, dstFloat, -1); // 重み付き和

		// 差分映像
		absdiff(srcFloat, dstFloat, diffFloat);

		// 2値化差分マスク映像
		diffFloat.convertTo(diff, CV_8UC3, 255.0); // 小数値化
		cvtColor(diff, binary, COLOR_BGR2GRAY); // グレースケール化
		GaussianBlur(binary, binary, Size(9, 9), 3.0, 3.0); // ノイズ除去(ぼかし)
		threshold(binary, binary, t_mask, 255, THRESH_BINARY); // 2値化
		erode(binary, binary, Mat(), Point(-1, -1), erosion); // 収縮
		imshow("差分マスク", binary);

		// 雪の発生
		if (currentFrame % 1 == 0) {
			Snow newSnow;
			newSnow.pos.x = randInt(0, cameraSize.width);
			newSnow.pos.y = 0;
			snows.push_back(newSnow);
		}

		// 雪の処理
		{
			int outFlag; // 削除フラグ
			int speed; // 移動速度
			auto itr = snows.begin();
			while (itr != snows.end()) {

				outFlag = 0;
				speed = 0;

				// 画面外判定
				if (
					(*itr).pos.x < 0 ||
					(*itr).pos.x >= cameraSize.width ||
					(*itr).pos.y < 0 ||
					(*itr).pos.y >= cameraSize.height
					) {
					outFlag = 1;
				}
				else {
					// 移動物体接触判定
					if (binary.at<unsigned char>((*itr).pos.y, (*itr).pos.x) == 255) {
						speed = 25;
					}

					// エッジ判定
					if (edge2.at<unsigned char>((*itr).pos.y, (*itr).pos.x) != 255) {
						speed = 3;
					}
				}

				(*itr).pos.y += speed;

				// 描画
				circle(src, (*itr).pos, 5, Scalar(255, 255, 255), FILLED, LINE_AA);

				// 削除 or イテレータを次に進める
				if (outFlag) {
					itr = snows.erase(itr);
				}
				else {
					itr++;
				}
			}
		}

		imshow("加工後映像", src);
		rec << src;

		if (waitKey(1000 / FPS) == 27) {
			break;
		}
  }

  return 0;
}

int randInt(int min, int max) {
	return (rand() % (max - min + 1)) + min;
}