#include "hpp/gui/attitude-device.h"

#include <QtConcurrentRun>

#include "hpp/gui/mainwindow.h"

AttitudeEventSender_deprecated::AttitudeEventSender_deprecated()
{
}

void AttitudeEventSender_deprecated::mouseEvent(const remoteimu::MouseEventSender::Event e)
{
  emit attitudeEvent(e.ori[0], e.ori[1], e.ori[2], e.ori[3]);
}

AttitudeDevice_deprecated::AttitudeDevice_deprecated () :
  port (6000),
  mouse_ (new remoteimu::UDPServer ("0.0.0.0", port), 5),
  aes_ (), mask (new hpp::boolSeq)
{
}

void AttitudeDevice_deprecated::init ()
{
  mouse_.setMouseEventSender(&aes_);

  mask->length(3);
  mask[0]=true;mask[1]=true;mask[2]=true;

  connect(&aes_, SIGNAL (attitudeEvent (double, double, double, double)),
          this, SLOT (updateJointAttitude (double, double, double, double)));

  MainWindow* m = MainWindow::instance();
  float black[4] = {1,1,1,1};
  m->osg()->addBox("hpp-gui/attitudeControl", 0.001f, 0.001f, 0.001f, black);
  m->osg()->addLandmark("hpp-gui/attitudeControl", 0.1f);
  m->osg()->setVisibility("hpp-gui/attitudeControl", "OFF");
}

void AttitudeDevice_deprecated::jointName (const std::string jointName)
{
  jn = jointName;
}

void AttitudeDevice_deprecated::start()
{
  if (lock_.isRunning()) stop ();

  MainWindow* m = MainWindow::instance();

  // Initialize the constraints
  hpp::Transform__var transform = m->hppClient()->robot()->getJointPosition(jn.c_str());
  hpp::floatSeq pos1, pos2; pos1.length(3); pos2.length(3);
  for (int i = 0; i < 3; i++) pos1[i] = 0;
  for (int i = 0; i < 3; i++) pos2[i] = transform.in()[i];
  for (int i = 0; i < 3; i++) frameViz[i]=(float)pos2[i];
  frameViz [3] = 1;
  for (int i = 0; i < 3; i++) frameViz[i+4]=0;

  mouse_.setInitialQuat (Eigen::Quaterniond(transform.in()[0+3],
                                          transform.in()[1+3],
                                          transform.in()[2+3],
                                          transform.in()[3+3]));

  m->hppClient()->problem()->createPositionConstraint (
        "attitudeDeviceControl/pos", "", jn.c_str(),
        pos2, pos1, mask.in());

  m->osg()->setVisibility("hpp-gui/attitudeControl", "ON");
  m->osg()->applyConfiguration("hpp-gui/attitudeControl", frameViz);
  MainWindow::JointMap::const_iterator it = m->jointMap ().find (jn);
  if (it != m->jointMap ().end())
    m->osg()->addLandmark(it->bodyName.c_str (), 0.05f);
  m->osg()->refresh();
  lock_ = QtConcurrent::run (&mouse_, &remoteimu::Mouse::handleEvents, true);
}

void AttitudeDevice_deprecated::stop()
{
  mouse_.stopEventHandler();
  MainWindow* m = MainWindow::instance();
  m->osg()->setVisibility("hpp-gui/attitudeControl", "OFF");
  MainWindow::JointMap::const_iterator it = m->jointMap ().find (jn);
  if (it != m->jointMap ().end())
    m->osg()->deleteLandmark(it->bodyName.c_str ());
  lock_.waitForFinished();
}

void AttitudeDevice_deprecated::updateJointAttitude(double w, double x, double y, double z)
{
  MainWindow* m = MainWindow::instance();

  q[0] = w; q[1] = x; q[2] = y; q[3] = z;
  for (int i = 0; i < 4; i++) {
      frameViz [i+3] = q [i];
    }
  m->osg()->applyConfiguration("hpp-gui/attitudeControl", frameViz);

  hpp::floatSeq pos1, pos2; pos1.length(3); pos2.length(3);
  for (int i = 0; i < 3; i++) pos1[i] = 0;
  for (int i = 0; i < 3; i++) pos2[i] = frameViz[i];

  m->hppClient()->problem()->createPositionConstraint (
        "attitudeDeviceControl/pos", "", jn.c_str(),
        pos2, pos1, mask.in());
  m->hppClient()->problem()->createOrientationConstraint (
        "attitudeDeviceControl/ori", "", jn.c_str(),
        q, mask.in());
  m->hppClient()->problem()->resetConstraints ();
  hpp::Names_t n; n.length(2);
  n[0] = "attitudeDeviceControl/ori"; n[1] = "attitudeDeviceControl/pos";
  m->hppClient()->problem()->setNumericalConstraints ("attitudeDeviceControl", n);
  hpp::floatSeq_var qin = m->hppClient()->robot()->getCurrentConfig ();
  hpp::floatSeq_var qproj;
  hpp::floatSeq_out qp_out (qproj);
  double err;
  bool res = m->hppClient()->problem()->applyConstraints (qin.in(), qp_out, err);
  if (!res) {
      qDebug() << "Projection failed: " << err;
      m->hppClient()->robot()->setCurrentConfig (qin.in());
  } else {
      m->hppClient()->robot()->setCurrentConfig (qproj);
  }
  m->applyCurrentConfiguration();
}

void AttitudeDevice_deprecated::updateTargetPosition (double x, double y, double z) {
  Eigen::Quaternion<double> q (frameViz[3], frameViz[4], frameViz[5], frameViz[6]);
  Eigen::Vector3d dx = q.inverse() * Eigen::Vector3d (x,y,z);
  frameViz[0] += dx[0];
  frameViz[1] += dx[1];
  frameViz[2] += dx[2];
}

AttitudeDeviceMsgBox_deprecated::AttitudeDeviceMsgBox_deprecated (QWidget *parent) :
   QMessageBox (QMessageBox::Information,  "Attitude Device","",
                QMessageBox::Close, parent,
                Qt::Dialog | Qt::WindowStaysOnTopHint),
  device_ ()
{
  setModal (false);
  setText ("Configuration steps:\n"
           "1 - Configure your device to send its datas to " + device_.address() + "\n"
           "2 - Do not move your device for a short initialization time (corresponding to 100 measurements of your device).\n"
           "3 - Move your device, the frame should move as well.\n"
           "4 - Close this popup to stop the connection."
           );
}

void AttitudeDeviceMsgBox_deprecated::show()
{
  connect (this, SIGNAL(finished(int)), &device_, SLOT (stop()));
  connect (this, SIGNAL(finished(int)), this, SLOT (deleteLater()));
  device_.init ();
  device_.start ();
  QMessageBox::show ();
}

void AttitudeDeviceMsgBox_deprecated::keyPressEvent(QKeyEvent *event)
{
  double shift = 0.01;
  if (event->modifiers() == Qt::ControlModifier)
    shift /= 10;
  else if (event->modifiers() == Qt::ShiftModifier)
    shift *= 10;
  switch (event->key ()) {
    case Qt::Key_Up:
    case Qt::Key_W:
      qDebug () << "X+";
      device_.updateTargetPosition (shift, 0, 0);
      break;
    case Qt::Key_Down:
    case Qt::Key_S:
      qDebug () << "X-";
      device_.updateTargetPosition (-shift, 0, 0);
      break;
    case Qt::Key_Left:
    case Qt::Key_A:
      qDebug () << "Y+";
      device_.updateTargetPosition (0, shift, 0);
      break;
    case Qt::Key_Right:
    case Qt::Key_D:
      qDebug () << "Y-";
      device_.updateTargetPosition (0, -shift, 0);
      break;
    case Qt::Key_PageUp:
    case Qt::Key_Q:
      qDebug () << "Z+";
      device_.updateTargetPosition (0, 0, shift);
      break;
    case Qt::Key_PageDown:
    case Qt::Key_E:
      qDebug () << "Z-";
      device_.updateTargetPosition (0, 0, -shift);
      break;
    }

  QMessageBox::keyPressEvent(event);
}
