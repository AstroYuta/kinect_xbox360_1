#include <iostream>
#include <sstream>

// NuiApi.h‚Ì‘O‚ÉWindows.h‚ðƒCƒ“ƒNƒ‹[ƒh‚·‚é
#include <Windows.h>
#include <NuiApi.h>

#include <opencv2/opencv.hpp>



#define ERROR_CHECK( ret )  \
  if ( ret != S_OK ) {    \
    std::stringstream ss;	\
    ss << "failed " #ret " " << std::hex << ret << std::endl;			\
    throw std::runtime_error( ss.str().c_str() );			\
  }

const NUI_IMAGE_RESOLUTION CAMERA_RESOLUTION = NUI_IMAGE_RESOLUTION_640x480;

class KinectSample
{
private:

  INuiSensor* kinect;
  HANDLE imageStreamHandle;
  HANDLE depthStreamHandle;
	HANDLE streamEvent;

  DWORD width;
  DWORD height;

public:

  KinectSample()
  {
  }

  ~KinectSample()
  {
    // I—¹ˆ—
    if ( kinect != 0 ) {
      kinect->NuiShutdown();
      kinect->Release();
    }
  }

  void initialize()
  {
    createInstance();

    // Kinect‚ÌÝ’è‚ð‰Šú‰»‚·‚é
    ERROR_CHECK( kinect->NuiInitialize( NUI_INITIALIZE_FLAG_USES_COLOR |
										NUI_INITIALIZE_FLAG_USES_DEPTH_AND_PLAYER_INDEX |
										NUI_INITIALIZE_FLAG_USES_SKELETON) );

    // RGBƒJƒƒ‰‚ð‰Šú‰»‚·‚é
    ERROR_CHECK( kinect->NuiImageStreamOpen( NUI_IMAGE_TYPE_COLOR, CAMERA_RESOLUTION,
      0, 2, 0, &imageStreamHandle ) );

    // ‹——£ƒJƒƒ‰‚ð‰Šú‰»‚·‚é
    ERROR_CHECK( kinect->NuiImageStreamOpen( NUI_IMAGE_TYPE_DEPTH_AND_PLAYER_INDEX, CAMERA_RESOLUTION,
      0, 2, 0, &depthStreamHandle ) );

    // Nearƒ‚[ƒh
    //ERROR_CHECK( kinect->NuiImageStreamSetImageFrameFlags(
    //  depthStreamHandle, NUI_IMAGE_STREAM_FLAG_ENABLE_NEAR_MODE )

	//スケルトンを初期化する
	ERROR_CHECK( kinect->NuiSkeletonTrackingEnable( 0, 0 ) );

    // ƒtƒŒ[ƒ€XVƒCƒxƒ“ƒg‚Ìƒnƒ“ƒhƒ‹‚ðì¬‚·‚é
    streamEvent = ::CreateEvent( 0, TRUE, FALSE, 0 );
    ERROR_CHECK( kinect->NuiSetFrameEndEvent( streamEvent, 0 ) );

    // Žw’è‚µ‚½‰ð‘œ“x‚ÌA‰æ–ÊƒTƒCƒY‚ðŽæ“¾‚·‚é
    ::NuiImageResolutionToSize(CAMERA_RESOLUTION, width, height );
  }

  void run()
  {
    cv::Mat image;

    // ƒƒCƒ“ƒ‹[ƒv
    while ( 1 ) {
      // ƒf[ƒ^‚ÌXV‚ð‘Ò‚Â
      DWORD ret = ::WaitForSingleObject( streamEvent, INFINITE );
      ::ResetEvent( streamEvent );

      drawRgbImage( image );
      drawDepthImage( image );
	  drawSkeleton( image );

      // ‰æ‘œ‚ð•\Ž¦‚·‚é
      cv::imshow( "RGBCamera", image );

      // I—¹‚Ì‚½‚ß‚ÌƒL[“ü—Íƒ`ƒFƒbƒNŒ“A•\Ž¦‚Ì‚½‚ß‚ÌƒEƒFƒCƒg
      int key = cv::waitKey( 10 );
      if ( key == 'q' ) {
        break;
      }
    }
  }

private:

  void createInstance()
  {
    // Ú‘±‚³‚ê‚Ä‚¢‚éKinect‚Ì”‚ðŽæ“¾‚·‚é
    int count = 0;
    ERROR_CHECK( ::NuiGetSensorCount( &count ) );
    if ( count == 0 ) {
      throw std::runtime_error( "Plese connect the kinect" );
    }

    // Å‰‚ÌKinect‚ÌƒCƒ“ƒXƒ^ƒ“ƒX‚ðì¬‚·‚é
    ERROR_CHECK( ::NuiCreateSensorByIndex( 0, &kinect ) );

    // Kinect‚Ìó‘Ô‚ðŽæ“¾‚·‚é
    HRESULT status = kinect->NuiStatus();
    if ( status != S_OK ) {
      throw std::runtime_error( "Kinect error" );
    }
  }

  void drawRgbImage( cv::Mat& image )
  {
      // RGBƒJƒƒ‰‚ÌƒtƒŒ[ƒ€ƒf[ƒ^‚ðŽæ“¾‚·‚é
      NUI_IMAGE_FRAME imageFrame = { 0 };
      ERROR_CHECK( kinect->NuiImageStreamGetNextFrame( imageStreamHandle, INFINITE, &imageFrame ) );

      // ‰æ‘œƒf[ƒ^‚ðŽæ“¾‚·‚é
      NUI_LOCKED_RECT colorData;
      imageFrame.pFrameTexture->LockRect( 0, &colorData, 0, 0 );

      // ‰æ‘œƒf[ƒ^‚ðƒRƒs[‚·‚é
      image = cv::Mat( height, width, CV_8UC4, colorData.pBits );

      // ƒtƒŒ[ƒ€ƒf[ƒ^‚ð‰ð•ú‚·‚é
      ERROR_CHECK( kinect->NuiImageStreamReleaseFrame( imageStreamHandle, &imageFrame ) );
  }

  void drawDepthImage( cv::Mat& image )
  {
      // ‹——£ƒJƒƒ‰‚ÌƒtƒŒ[ƒ€ƒf[ƒ^‚ðŽæ“¾‚·‚é
      NUI_IMAGE_FRAME depthFrame = { 0 };
      ERROR_CHECK( kinect->NuiImageStreamGetNextFrame( depthStreamHandle, INFINITE, &depthFrame ) );

      // ‹——£ƒf[ƒ^‚ðŽæ“¾‚·‚é
      NUI_LOCKED_RECT depthData = { 0 };
      depthFrame.pFrameTexture->LockRect( 0, &depthData, 0, 0 );

      USHORT* depth = (USHORT*)depthData.pBits;
      for ( int i = 0; i < (depthData.size / sizeof(USHORT)); ++i ) {
        USHORT distance = ::NuiDepthPixelToDepth( depth[i] );

        LONG depthX = i % width;
        LONG depthY = i / width;
        LONG colorX = depthX;
        LONG colorY = depthY;

        // ‹——£ƒJƒƒ‰‚ÌÀ•W‚ðARGBƒJƒƒ‰‚ÌÀ•W‚É•ÏŠ·‚·‚é
        kinect->NuiImageGetColorPixelCoordinatesFromDepthPixelAtResolution(
          CAMERA_RESOLUTION, CAMERA_RESOLUTION,
          0, depthX , depthY, depth[i], &colorX, &colorY );

        // ˆê’èˆÈã‚Ì‹——£‚ð•`‰æ‚µ‚È‚¢
        if ( distance >= 1000 ) {
          int index = ((colorY * width) + colorX) * 4;
          UCHAR* data = &image.data[index];
          data[0] = 255;
          data[1] = 255;
          data[2] = 255;
        }
      }

      // ƒtƒŒ[ƒ€ƒf[ƒ^‚ð‰ð•ú‚·‚é
      ERROR_CHECK( kinect->NuiImageStreamReleaseFrame( depthStreamHandle, &depthFrame ) );
  }
};

void main()
{

  try {
    KinectSample kinect;
    kinect.initialize();
    kinect.run();
  }
  catch ( std::exception& ex ) {
    std::cout << ex.what() << std::endl;
  }
}
