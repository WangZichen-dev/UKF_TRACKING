#include "ukf.h"
#include "Eigen/Dense"

using Eigen::MatrixXd;
using Eigen::VectorXd;
 
/**
 * Initializes Unscented Kalman filter
 */
UKF::UKF() {
  // if this is false, laser measurements will be ignored (except during init)
  use_laser_ = true;

  // if this is false, radar measurements will be ignored (except during init)
  use_radar_ = true;

  n_x_ = 5;
  n_aug_ = n_x_+2;
 
  // initial state vector
  x_ = VectorXd(n_x_);

  // initial covariance matrix
  P_ = MatrixXd(n_x_, n_x_);

  // init predicted sigma point
  Xsig_pred_ = MatrixXd(n_x_, 2*n_aug_+1);

  // Process noise standard deviation longitudinal acceleration in m/s^2
  std_a_ = 3;

  // Process noise standard deviation yaw acceleration in rad/s^2
  std_yawdd_ = 2.;
  
  /**
   * DO NOT MODIFY measurement noise values below.
   * These are provided by the sensor manufacturer.
   */

  // Laser measurement noise standard deviation position1 in m
  std_laspx_ = 0.15;

  // Laser measurement noise standard deviation position2 in m
  std_laspy_ = 0.15;

  // Radar measurement noise standard deviation radius in m
  std_radr_ = 0.3;

  // Radar measurement noise standard deviation angle in rad
  std_radphi_ = 0.03;

  // Radar measurement noise standard deviation radius change in m/s
  std_radrd_ = 0.3;
  
  /**
   * End DO NOT MODIFY section for measurement noise values 
   */
  
  /**
   * TODO: Complete the initialization. See ukf.h for other member properties.
   * Hint: one or more values initialized above might be wildly off...
   */
  is_initialized_ = false;

  lambda_ = 3 - n_x_;
  weights_ = VectorXd(2*n_aug_+1);
  weights_.fill(0.5/(lambda_+n_aug_));
  weights_(0) = lambda_ / (lambda_+n_aug_);
}

UKF::~UKF() {}

void UKF::ProcessMeasurement(MeasurementPackage meas_package) {
    /**
   * TODO: Complete this function! Make sure you switch between lidar and radar
   * measurements.
   */
  // if ukf is not initialized, init x_ and P_ with input sensor message
  // if ukf is initialized, fuse motion model prediction and sensor measurement
  std::cout<<"current time: "<<meas_package.timestamp_/1000000.0<<std::endl;

  if(!is_initialized_){
    if(meas_package.sensor_type_ == MeasurementPackage::LASER){
      x_ << meas_package.raw_measurements_(0), meas_package.raw_measurements_(1), 0., 0., 0.;
      P_ << std_laspx_*std_laspx_,0,0,0,0,
            0,std_laspy_*std_laspy_,0,0,0,
            0,0,1,0,0,
            0,0,0,1,0,
            0,0,0,0,1;
      std::cout<<"init with lidar input"<<std::endl;
    }
    else if(meas_package.sensor_type_ == MeasurementPackage::RADAR){
      double r = meas_package.raw_measurements_(0);
      double phi = meas_package.raw_measurements_(1);
      double r_dot = meas_package.raw_measurements_(2);

      double px = r*cos(phi);
      double py = r*sin(phi);
      double vx = r_dot*cos(phi);
      double vy = r_dot*sin(phi);

      x_ << r*cos(phi), r*sin(phi), sqrt(vx*vx + vy*vy), 0, 0;
      P_ << std_radr_*std_radr_,0,0,0,0,
            0,std_radr_*std_radr_,0,0,0,
            0,0,std_radrd_*std_radrd_,0,0,
            0,0,0,std_radphi_*std_radphi_,0,
            0,0,0,0,std_radphi_*std_radphi_;
      std::cout<<"init with radar input"<<std::endl;
    }
    time_us_ = meas_package.timestamp_;
    is_initialized_ = true;
    std::cout<<"init x: "<<x_<<std::endl;
    //std::cout<<"init P: "<<P_<<std::endl;
    return;
  }
  
  // predict simga points
  double delta_t = (meas_package.timestamp_ - time_us_)/1000000.0;

  if(meas_package.sensor_type_ == MeasurementPackage::LASER && use_laser_==true){
    Prediction(delta_t);
    std::cout<<"predict x: "<<x_<<std::endl;
    //std::cout<<"predict P: "<<P_<<std::endl;

    UpdateLidar(meas_package);
    std::cout<<"lidar update x: "<<x_<<std::endl;
    //std::cout<<"lidar update P: "<<P_<<std::endl;

    time_us_ = meas_package.timestamp_;
  }
  else if(meas_package.sensor_type_ == MeasurementPackage::RADAR && use_radar_==true){
    Prediction(delta_t);
    std::cout<<"predict x: "<<x_<<std::endl;
    //std::cout<<"predict P: "<<P_<<std::endl;
    
    UpdateRadar(meas_package);
    std::cout<<"radar update x: "<<x_<<std::endl;
    //std::cout<<"radar update P: "<<P_<<std::endl;

    time_us_ = meas_package.timestamp_;
  }
}

void UKF::Prediction(double delta_t) {
  /**
   * TODO: Complete this function! Estimate the object's location. 
   * Modify the state vector, x_. Predict sigma points, the state, 
   * and the state covariance matrix.
   */
  // generate augmented sigma points
  // predict sigma points
  // predict mean and coveriance
  
  // create augmented mean vector
  VectorXd x_aug = VectorXd(n_aug_);
  // create augmented state covariance
  MatrixXd P_aug = MatrixXd(n_aug_, n_aug_);
  // create sigma point matrix
  MatrixXd Xsig_aug = MatrixXd(n_aug_, 2 * n_aug_ + 1);
  Xsig_aug.fill(0.0);
  // create augmented mean state
  x_aug.head(5) = x_;
  x_aug(5) = 0;
  x_aug(6) = 0;
  // create augmented covariance matrix
  P_aug.fill(0.0);
  P_aug.topLeftCorner(5,5) = P_;
  P_aug(5,5) = std_a_*std_a_;
  P_aug(6,6) = std_yawdd_*std_yawdd_;
  // create square root matrix
  MatrixXd L = P_aug.llt().matrixL();
  // create augmented sigma points
  Xsig_aug.col(0)  = x_aug;
  for (int i = 0; i< n_aug_; ++i) {
    Xsig_aug.col(i+1) = x_aug + sqrt(lambda_+n_aug_) * L.col(i);
    Xsig_aug.col(i+1+n_aug_) = x_aug - sqrt(lambda_+n_aug_) * L.col(i);
  }

  // predict sigma points
  for (int i = 0; i< 2*n_aug_+1; ++i) {
    // extract values for better readability
    double p_x = Xsig_aug(0,i);
    double p_y = Xsig_aug(1,i);
    double v = Xsig_aug(2,i);
    double yaw = Xsig_aug(3,i);
    double yawd = Xsig_aug(4,i);
    double nu_a = Xsig_aug(5,i);
    double nu_yawdd = Xsig_aug(6,i);
    // predicted state values
    double px_p, py_p;
    // avoid division by zero
    if (fabs(yawd) > 0.001) {
        px_p = p_x + v/yawd * ( sin (yaw + yawd*delta_t) - sin(yaw));
        py_p = p_y + v/yawd * ( cos(yaw) - cos(yaw+yawd*delta_t) );
    } else {
        px_p = p_x + v*delta_t*cos(yaw);
        py_p = p_y + v*delta_t*sin(yaw);
    }
    double v_p = v;
    double yaw_p = yaw + yawd*delta_t;
    double yawd_p = yawd;
    // add noise
    px_p = px_p + 0.5*nu_a*delta_t*delta_t * cos(yaw);
    py_p = py_p + 0.5*nu_a*delta_t*delta_t * sin(yaw);
    v_p = v_p + nu_a*delta_t;
    yaw_p = yaw_p + 0.5*nu_yawdd*delta_t*delta_t;
    yawd_p = yawd_p + nu_yawdd*delta_t;
    // write predicted sigma point into right column
    Xsig_pred_(0,i) = px_p;
    Xsig_pred_(1,i) = py_p;
    Xsig_pred_(2,i) = v_p;
    Xsig_pred_(3,i) = yaw_p;
    Xsig_pred_(4,i) = yawd_p;
  }

  
  // predicted state mean
  x_.fill(0.0);
  for (int i = 0; i < 2 * n_aug_ + 1; ++i) {  // iterate over sigma points
    x_ = x_ + weights_(i) * Xsig_pred_.col(i);
  }
  // predicted state covariance matrix
  P_.fill(0.0);
  for (int i = 0; i < 2 * n_aug_ + 1; ++i) {  // iterate over sigma points
    // state difference
    VectorXd x_diff = Xsig_pred_.col(i) - x_;
    // angle normalization
    while (x_diff(3)> M_PI) x_diff(3)-=2.*M_PI;
    while (x_diff(3)<-M_PI) x_diff(3)+=2.*M_PI;
    P_ = P_ + weights_(i) * x_diff * x_diff.transpose() ;
  }
  
}

void UKF::UpdateLidar(MeasurementPackage meas_package) {
  /**
   * TODO: Complete this function! Use lidar data to update the belief 
   * about the object's position. Modify the state vector, x_, and 
   * covariance, P_.
   * You can also calculate the lidar NIS, if desired.
   */

  // transform predicted sigma points to measurement space
  // calculate predicted measurement
  // calculate S and TC
  // calculate kalman gain
  // update x_ and P_
  
  // set measurement dimension, radar can measure px and py
  int n_z = 2;
  // create matrix for sigma points in measurement space
  MatrixXd Zsig = MatrixXd(n_z, 2 * n_aug_ + 1);
  // mean predicted measurement
  VectorXd z_pred = VectorXd(n_z);
  // measurement covariance matrix S
  MatrixXd S = MatrixXd(n_z,n_z);
  // create matrix for cross correlation Tc
  MatrixXd Tc = MatrixXd(n_x_, n_z);
  // transform sigma points into measurement space
  for (int i = 0; i < 2 * n_aug_ + 1; ++i) {  // 2n+1 simga points
    // extract values for better readability
    double p_x = Xsig_pred_(0,i);
    double p_y = Xsig_pred_(1,i);

    // measurement model
    Zsig(0,i) = p_x;
    Zsig(1,i) = p_y;
  }
  // mean predicted measurement
  z_pred.fill(0.0);
  for (int i=0; i < 2*n_aug_+1; ++i) {
    z_pred = z_pred + weights_(i) * Zsig.col(i);
  }
  // innovation covariance matrix S
  S.fill(0.0);
  for (int i = 0; i < 2 * n_aug_ + 1; ++i) {  // 2n+1 simga points
    // residual
    VectorXd z_diff = Zsig.col(i) - z_pred;
    S = S + weights_(i) * z_diff * z_diff.transpose();
  }
  // add measurement noise covariance matrix
  MatrixXd R = MatrixXd(n_z,n_z);
  R <<  std_laspx_*std_laspx_, 0,
        0, std_laspy_*std_laspy_;
  S = S + R;

  
  // calculate cross correlation matrix
  Tc.fill(0.0);
  for (int i = 0; i < 2 * n_aug_ + 1; ++i) {  // 2n+1 simga points
    // residual
    VectorXd z_diff = Zsig.col(i) - z_pred;
    // state difference
    VectorXd x_diff = Xsig_pred_.col(i) - x_;
    // angle normalization
    while (x_diff(3)> M_PI) x_diff(3)-=2.*M_PI;
    while (x_diff(3)<-M_PI) x_diff(3)+=2.*M_PI;
    Tc = Tc + weights_(i) * x_diff * z_diff.transpose();
  }

  // Kalman gain K;
  MatrixXd K = Tc * S.inverse();

  VectorXd z_measure = VectorXd(2);
  z_measure << meas_package.raw_measurements_(0), meas_package.raw_measurements_(1);
  // residual
  VectorXd z_diff = z_measure - z_pred;

  // update state mean and covariance matrix
  x_ = x_ + K * z_diff;
  P_ = P_ - K*S*K.transpose();
}

void UKF::UpdateRadar(MeasurementPackage meas_package) {
  /**
   * TODO: Complete this function! Use radar data to update the belief 
   * about the object's position. Modify the state vector, x_, and 
   * covariance, P_.
   * You can also calculate the radar NIS, if desired.
   */

  // transform predicted sigma points to measurement space
  // calculate predicted measurement
  // calculate S and TC
  // calculate kalman gain
  // update x_ and P_
  
  // set measurement dimension, radar can measure r, phi, and r_dot
  int n_z = 3;
  // create matrix for sigma points in measurement space
  MatrixXd Zsig = MatrixXd(n_z, 2 * n_aug_ + 1);
  // mean predicted measurement
  VectorXd z_pred = VectorXd(n_z);
  // measurement covariance matrix S
  MatrixXd S = MatrixXd(n_z,n_z);
  // create matrix for cross correlation Tc
  MatrixXd Tc = MatrixXd(n_x_, n_z);
  // transform sigma points into measurement space
  for (int i = 0; i < 2 * n_aug_ + 1; ++i) {  // 2n+1 simga points
    // extract values for better readability
    double p_x = Xsig_pred_(0,i);
    double p_y = Xsig_pred_(1,i);
    double v  = Xsig_pred_(2,i);
    double yaw = Xsig_pred_(3,i);
    double v1 = cos(yaw)*v;
    double v2 = sin(yaw)*v;
    // measurement model
    Zsig(0,i) = sqrt(p_x*p_x + p_y*p_y);                       // r
    Zsig(1,i) = atan2(p_y,p_x);                                // phi
    Zsig(2,i) = (p_x*v1 + p_y*v2) / sqrt(p_x*p_x + p_y*p_y);   // r_dot
  }
  // mean predicted measurement
  z_pred.fill(0.0);
  for (int i=0; i < 2*n_aug_+1; ++i) {
    z_pred = z_pred + weights_(i) * Zsig.col(i);
  }
  // innovation covariance matrix S
  S.fill(0.0);
  for (int i = 0; i < 2 * n_aug_ + 1; ++i) {  // 2n+1 simga points
    // residual
    VectorXd z_diff = Zsig.col(i) - z_pred;
    // angle normalization
    while (z_diff(1)> M_PI) z_diff(1)-=2.*M_PI;
    while (z_diff(1)<-M_PI) z_diff(1)+=2.*M_PI;
    S = S + weights_(i) * z_diff * z_diff.transpose();
  }
  // add measurement noise covariance matrix
  MatrixXd R = MatrixXd(n_z,n_z);
  R <<  std_radr_*std_radr_, 0, 0,
        0, std_radphi_*std_radphi_, 0,
        0, 0,std_radrd_*std_radrd_;
  S = S + R;

  
  // calculate cross correlation matrix
  Tc.fill(0.0);
  for (int i = 0; i < 2 * n_aug_ + 1; ++i) {  // 2n+1 simga points
    // residual
    VectorXd z_diff = Zsig.col(i) - z_pred;
    // angle normalization
    while (z_diff(1)> M_PI) z_diff(1)-=2.*M_PI;
    while (z_diff(1)<-M_PI) z_diff(1)+=2.*M_PI;
    // state difference
    VectorXd x_diff = Xsig_pred_.col(i) - x_;
    // angle normalization
    while (x_diff(3)> M_PI) x_diff(3)-=2.*M_PI;
    while (x_diff(3)<-M_PI) x_diff(3)+=2.*M_PI;
    Tc = Tc + weights_(i) * x_diff * z_diff.transpose();
  }

  // Kalman gain K;
  MatrixXd K = Tc * S.inverse();

  VectorXd z_measure = VectorXd(3);
  z_measure << meas_package.raw_measurements_(0), meas_package.raw_measurements_(1), meas_package.raw_measurements_(2);
  // residual
  VectorXd z_diff = z_measure - z_pred;

  // angle normalization
  while (z_diff(1)> M_PI) z_diff(1)-=2.*M_PI;
  while (z_diff(1)<-M_PI) z_diff(1)+=2.*M_PI;

  // update state mean and covariance matrix
  x_ = x_ + K * z_diff;
  P_ = P_ - K*S*K.transpose();
}