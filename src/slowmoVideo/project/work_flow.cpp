/*
 * precalculate optical flow
 * 2014 Valery Brasseur <vbrasseur@gmail.com>
 */

#include "flowSourceOpenCV_sV.h"
#include "project_sV.h"
#include "abstractFrameSource_sV.h"
#include "../lib/flowRW_sV.h"
#include "../lib/flowField_sV.h"

#include "opencv2/video/tracking.hpp"
#include "opencv2/imgproc/imgproc.hpp"
#include "opencv2/highgui/highgui.hpp"


#include "work_flow.h"
#include <QTimer>
#include <QEventLoop>

#include <QThread>
#include <QDebug>

using namespace cv;

extern void drawOptFlowMap(const Mat& flow, int step,
                         double, const Scalar& color, std::string flowname );



WorkerFlow::WorkerFlow(QObject *parent) :
QObject(parent)
{
    _working =false;
    _abort = false;
    
}

void WorkerFlow::requestWork()
{
    mutex.lock();
    _working = true;
    _abort = false;
    qDebug()<<"OpticalFlow worker start in Thread "<<thread()->currentThreadId();
    mutex.unlock();
    
    emit workFlowRequested();
}

void WorkerFlow::abort()
{
    mutex.lock();
    if (_working) {
        _abort = true;
        qDebug()<<"OpticalFlow worker aborting in Thread "<<thread()->currentThreadId();
    }
    mutex.unlock();
}


//TODO: should get an object Flow...
void WorkerFlow::setFrameSize(FrameSize _frameSize)
{
    frameSize = _frameSize;
}

void WorkerFlow::setProject(Project_sV *_project)
{
    project = _project;
}


void WorkerFlow::setFlowSource(AbstractFlowSource_sV* _flowsource)
{
    flowSource = _flowsource;
}


//TODO: call abstractflow source method
/**
 *  doWorkFlow
 * call optical flow on each frame
 */
void WorkerFlow::doWorkFlow()
{
    qDebug()<<"Starting OpticalFlow process in Thread "<<thread()->currentThreadId();
    
    int lastFrame;
    int frame;
    int startFrame;
    int next = 0;
    int nextFrame;
    
    /* out of loop work */
    lastFrame = project->frameSource()->framesCount();
    startFrame = frame = 0;

    if (forward) {
    	qDebug() << "forward flow";
        next = 1;
    } else {
    	qDebug() << "backward flow";
        next = -1;
    }
    Mat prevgray, gray, flow;
    
    qDebug() << "Pre Building forward flow for Size: " << frameSize;
    
    // load first frame
    QString prevpath = project->frameSource()->framePath(frame, frameSize);
    prevgray = imread(prevpath.toStdString(), 0);
    
    // TODO: need sliders for all these parameters
    
    const int levels = 3;
    const int winsize = 15;
    const int iterations = 8;
    
    const double polySigma = 1.2;
    const double pyrScale = 0.5; // classical pyr
    const int polyN = 5;
    int flags = 0;
    
    /* real workhorse */
    for(frame=0;frame<lastFrame;frame++) {
        
        
        // Checks if the process should be aborted
        mutex.lock();
        bool abort = _abort;
        mutex.unlock();
        
        if (abort) {
            qDebug()<<"Aborting OpticalFlow process in Thread "<<thread()->currentThreadId();
            break;
        }
        
        nextFrame = frame + next;
        
        QString flowFileName(flowSource->flowPath(frame, nextFrame, frameSize));
        
        qDebug() << "Building flow for left frame " << frame << " to right frame " << nextFrame << "; Size: " << frameSize;
        /// \todo Check if size is equal
        if (!QFile(flowFileName).exists()) {
            
            //QTime time;
            //time.start();
            
            QString prevpath = project->frameSource()->framePath(frame, frameSize);
            QString path = project->frameSource()->framePath(nextFrame, frameSize);
            
            gray = imread(path.toStdString(), 0);
            
            // use previous flow info
            //if (frame!=0)
            //    flags |= OPTFLOW_USE_INITIAL_FLOW;
#if 0            
            calcOpticalFlowFarneback(
                                     prevgray, gray,
                                     flow,
                                     pyrScale, //0.5,
                                     levels, //3,
                                     winsize, //15,
                                     iterations, //3,
                                     polyN, //5,
                                     polySigma, //1.2,
                                     flags //0 OPTFLOW_USE_INITIAL_FLOW
                                     );
            // save result
            drawOptFlowMap(flow, 1, 1.5, CV_RGB(0, 255, 0), flowFileName.toStdString());
#else
	// test the work horse
            qDebug() << "dummy Optical flow built for " << flowFileName;
#endif
            std::swap(prevgray, gray);
            qDebug() << "Optical flow built for " << flowFileName;
            
        } else {
            qDebug().nospace() << "Re-using existing flow image for left frame " << frame << " to right frame " << nextFrame << ": " << flowFileName;
        }
        //qDebug() << "Optical flow built for " << flowFileName << " in " << time.elapsed() << " ms.";
        // Once we're done waiting, value is updated
        emit valueChanged(QString::number(frame));
        
    } /* for */
    
    
    // Set _working to false, meaning the process can't be aborted anymore.
    mutex.lock();
    _working = false;
    mutex.unlock();
    
    qDebug()<<"OpticalFlow process finished in Thread "<<thread()->currentThreadId();
    
    // the finished signal is sent
    emit finished();
}
