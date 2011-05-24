/*
 * Software License Agreement (BSD License)
 *
 *  Copyright (c) 2010, Willow Garage, Inc.
 *  All rights reserved.
 *
 *  Redistribution and use in source and binary forms, with or without
 *  modification, are permitted provided that the following conditions
 *  are met:
 *
 *   * Redistributions of source code must retain the above copyright
 *     notice, this list of conditions and the following disclaimer.
 *   * Redistributions in binary form must reproduce the above
 *     copyright notice, this list of conditions and the following
 *     disclaimer in the documentation and/or other materials provided
 *     with the distribution.
 *   * Neither the name of Willow Garage, Inc. nor the names of its
 *     contributors may be used to endorse or promote products derived
 *     from this software without specific prior written permission.
 *
 *  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 *  "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 *  LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
 *  FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
 *  COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
 *  INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
 *  BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 *  LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
 *  CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 *  LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN
 *  ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 *  POSSIBILITY OF SUCH DAMAGE.
 *
 */

#include "pcl/features/integral_image_normal.h"
#include "pcl/features/normal_3d.h"

#include "pcl/win32_macros.h"

#define SQR(a) ((a)*(a))

//////////////////////////////////////////////////////////////////////////////////////////////
pcl::IntegralImageNormalEstimation::IntegralImageNormalEstimation ()
: integral_image_x_(NULL),
  integral_image_y_(NULL),
  integral_image_xyz_(NULL),
  integral_image_(NULL),
  diff_x_(NULL),
  diff_y_(NULL),
  depth_data_(NULL)
{
}

//////////////////////////////////////////////////////////////////////////////////////////////
pcl::IntegralImageNormalEstimation::~IntegralImageNormalEstimation ()
{
  if (integral_image_x_ != NULL) delete integral_image_x_;
  if (integral_image_y_ != NULL) delete integral_image_y_;
  if (integral_image_xyz_ != NULL) delete integral_image_xyz_;
  if (integral_image_ != NULL) delete integral_image_;
  if (diff_x_ != NULL) delete diff_x_;
  if (diff_y_ != NULL) delete diff_y_;
  if (depth_data_ != NULL) delete depth_data_;
}

//////////////////////////////////////////////////////////////////////////////////////////////
void
pcl::IntegralImageNormalEstimation::setInputData (
  float * data,
  const int width, const int height,
  const int dimensions, const int element_stride,
  const int row_stride, const float distance_threshold,
  const NormalEstimationMethod normal_estimation_method )
{
  data_ = data;
  width_ = width;
  height_ = height;
  dimensions_ = dimensions;
  element_stride_ = element_stride;
  row_stride_ = row_stride;


  distance_threshold_ = distance_threshold;

  normal_estimation_method_ = normal_estimation_method;

  // compute derivatives
  if (integral_image_x_ != NULL) delete integral_image_x_;
  if (integral_image_y_ != NULL) delete integral_image_y_;
  if (integral_image_xyz_ != NULL) delete integral_image_xyz_;
  if (integral_image_ != NULL) delete integral_image_;
  if (diff_x_ != NULL) delete diff_x_;
  if (diff_y_ != NULL) delete diff_y_;
  if (depth_data_ != NULL) delete depth_data_;

  if (normal_estimation_method == COVARIANCE_MATRIX)
  {
    // compute integral images
    integral_image_xyz_ = new ::pcl::IntegralImage2D<float, double>(
      data,
      width_,
      height_,
      3,
      true,
      element_stride,
      row_stride );
  }
  else if (normal_estimation_method == AVERAGE_3D_GRADIENT)
  {
    diff_x_ = new float[4*width_*height_];
    diff_y_ = new float[4*width_*height_];

    memset(diff_x_, 0, sizeof(float)*4*width_*height_);
    memset(diff_y_, 0, sizeof(float)*4*width_*height_);

    for (int row_index = 1; row_index < height_-1; ++row_index)
    {
      float * data_pointer_y_up = data_ + (row_index-1)*row_stride_ + element_stride_;
      float * data_pointer_y_down = data_ + (row_index+1)*row_stride_ + element_stride_;
      float * data_pointer_x_left = data_ + row_index*row_stride_;
      float * data_pointer_x_right = data_ + row_index*row_stride_ + 2*element_stride_;

      float * diff_x_pointer = diff_x_ + row_index*4*width_ + 4;
      float * diff_y_pointer = diff_y_ + row_index*4*width_ + 4;

      for (int col_index = 1; col_index < width_-1; ++col_index)
      {
        {      
          diff_x_pointer[0] = data_pointer_x_right[0]-data_pointer_x_left[0];
          diff_x_pointer[1] = data_pointer_x_right[1]-data_pointer_x_left[1];
          diff_x_pointer[2] = data_pointer_x_right[2]-data_pointer_x_left[2];    
        }

        {      
          diff_y_pointer[0] = data_pointer_y_down[0]-data_pointer_y_up[0];
          diff_y_pointer[1] = data_pointer_y_down[1]-data_pointer_y_up[1];
          diff_y_pointer[2] = data_pointer_y_down[2]-data_pointer_y_up[2];
        }

        diff_x_pointer += 4;
        diff_y_pointer += 4;

        data_pointer_y_up += element_stride_;
        data_pointer_y_down += element_stride_;
        data_pointer_x_left += element_stride_;
        data_pointer_x_right += element_stride_;
      }
    }


    // compute integral images
    integral_image_x_ = new ::pcl::IntegralImage2D<float, double>(
      diff_x_,
      width_,
      height_,
      3,
      false,
      4,
      4*width_ );

    integral_image_y_ = new ::pcl::IntegralImage2D<float, double>(
      diff_y_,
      width_,
      height_,
      3,
      false,
      4,
      4*width_ );
  }
  //else if (normal_estimation_method == AVERAGE_DEPTH_CHANGE)
  //{
  //  diff_x_ = new float[width_*height_];
  //  diff_y_ = new float[width_*height_];

  //  memset(diff_x_, 0, sizeof(float)*width_*height_);
  //  memset(diff_y_, 0, sizeof(float)*width_*height_);

  //  for (int row_index = 1; row_index < height_-1; ++row_index)
  //  {
  //    float * data_pointer_y_up = data_ + (row_index-1)*row_stride_ + element_stride_;
  //    float * data_pointer_y_down = data_ + (row_index+1)*row_stride_ + element_stride_;
  //    float * data_pointer_x_left = data_ + row_index*row_stride_;
  //    float * data_pointer_x_right = data_ + row_index*row_stride_ + 2*element_stride_;

  //    float * diff_x_pointer = diff_x_ + row_index*width_ + 1;
  //    float * diff_y_pointer = diff_y_ + row_index*width_ + 1;

  //    for (int col_index = 1; col_index < width_-1; ++col_index)
  //    {
  //      {      
  //        *diff_x_pointer = data_pointer_x_right[2]-data_pointer_x_left[2];
  //      }

  //      {      
  //        *diff_y_pointer = data_pointer_y_down[2]-data_pointer_y_up[2];
  //      }

  //      ++diff_x_pointer;
  //      ++diff_y_pointer;

  //      data_pointer_y_up += element_stride_;
  //      data_pointer_y_down += element_stride_;
  //      data_pointer_x_left += element_stride_;
  //      data_pointer_x_right += element_stride_;
  //    }
  //  }


  //  // compute integral images
  //  integral_image_x_ = new ::pcl::IntegralImage2D<float, double>(
  //    diff_x_,
  //    width_,
  //    height_,
  //    1,
  //    false,
  //    1,
  //    width_ );

  //  integral_image_y_ = new ::pcl::IntegralImage2D<float, double>(
  //    diff_y_,
  //    width_,
  //    height_,
  //    1,
  //    false,
  //    1,
  //    width_ );
  //}
  else if (normal_estimation_method == AVERAGE_DEPTH_CHANGE)
  {
    // compute integral image
    integral_image_ = new ::pcl::IntegralImage2D<float, double>(
      &(data_[2]),
      width_,
      height_,
      1,
      false,
      element_stride,
      row_stride );
  }
}


//////////////////////////////////////////////////////////////////////////////////////////////
void
pcl::IntegralImageNormalEstimation::setRectSize (const int width, const int height)
{
  rect_width_ = width;
  rect_height_ = height;
}

//////////////////////////////////////////////////////////////////////////////////////////////
pcl::Normal
pcl::IntegralImageNormalEstimation::compute (const int pos_x, const int pos_y)
{
  if (normal_estimation_method_ == COVARIANCE_MATRIX)
  {
    const float mean_x = integral_image_xyz_->getSum(pos_x-rect_width_/2, pos_y-rect_height_/2, rect_width_, rect_height_, 0);
    const float mean_y = integral_image_xyz_->getSum(pos_x-rect_width_/2, pos_y-rect_height_/2, rect_width_, rect_height_, 1);
    const float mean_z = integral_image_xyz_->getSum(pos_x-rect_width_/2, pos_y-rect_height_/2, rect_width_, rect_height_, 2);

    const float mean_xx = integral_image_xyz_->getSum(pos_x-rect_width_/2, pos_y-rect_height_/2, rect_width_, rect_height_, 0, 0);
    const float mean_xy = integral_image_xyz_->getSum(pos_x-rect_width_/2, pos_y-rect_height_/2, rect_width_, rect_height_, 0, 1);
    const float mean_xz = integral_image_xyz_->getSum(pos_x-rect_width_/2, pos_y-rect_height_/2, rect_width_, rect_height_, 0, 2);
    const float mean_yx = mean_xy;
    const float mean_yy = integral_image_xyz_->getSum(pos_x-rect_width_/2, pos_y-rect_height_/2, rect_width_, rect_height_, 1, 1);
    const float mean_yz = integral_image_xyz_->getSum(pos_x-rect_width_/2, pos_y-rect_height_/2, rect_width_, rect_height_, 1, 2);
    const float mean_zx = mean_xz;
    const float mean_zy = mean_yz;
    const float mean_zz = integral_image_xyz_->getSum(pos_x-rect_width_/2, pos_y-rect_height_/2, rect_width_, rect_height_, 2, 2);


    EIGEN_ALIGN16 Eigen::Matrix3f covariance_matrix;
    covariance_matrix(0, 0) = mean_xx;
    covariance_matrix(0, 1) = mean_xy;
    covariance_matrix(0, 2) = mean_xz;
    covariance_matrix(1, 0) = mean_yx;
    covariance_matrix(1, 1) = mean_yy;
    covariance_matrix(1, 2) = mean_yz;
    covariance_matrix(2, 0) = mean_zx;
    covariance_matrix(2, 1) = mean_zy;
    covariance_matrix(2, 2) = mean_zz;

    Eigen::Vector3f center;
    center(0, 0) = mean_x;
    center(1, 0) = mean_y;
    center(2, 0) = mean_z;


    covariance_matrix -= (1.0f/(rect_width_*rect_height_)) * (center * center.transpose());
    covariance_matrix *= 1.0f/(rect_width_*rect_height_-1);
    
    EIGEN_ALIGN16 Eigen::Vector3f eigen_values;
    EIGEN_ALIGN16 Eigen::Matrix3f eigen_vectors;
    pcl::eigen33 (covariance_matrix, eigen_vectors, eigen_values);
  
    float normal_x = eigen_vectors(0, 0);
    float normal_y = eigen_vectors(1, 0);
    float normal_z = eigen_vectors(2, 0);

    if (normal_z > 0.0f)
    {
      normal_x *= -1.0f;
      normal_y *= -1.0f;
      normal_z *= -1.0f;
    }

    const float normal_length = sqrt(SQR(normal_x) + SQR(normal_y) + SQR(normal_z));
    const float scale = 1.0f/normal_length;
    
    pcl::Normal normal;
    normal.normal_x = normal_x*scale;
    normal.normal_y = normal_y*scale;
    normal.normal_z = normal_z*scale;
    normal.curvature = 0.0f;

    return normal;
  }
  else if (normal_estimation_method_ == AVERAGE_3D_GRADIENT)
  {
    const float mean_x_x = integral_image_x_->getSum(pos_x-rect_width_/2, pos_y-rect_height_/2, rect_width_, rect_height_, 0);
    const float mean_x_y = integral_image_x_->getSum(pos_x-rect_width_/2, pos_y-rect_height_/2, rect_width_, rect_height_, 1);
    const float mean_x_z = integral_image_x_->getSum(pos_x-rect_width_/2, pos_y-rect_height_/2, rect_width_, rect_height_, 2);

    const float mean_y_x = integral_image_y_->getSum(pos_x-rect_width_/2, pos_y-rect_height_/2, rect_width_, rect_height_, 0);
    const float mean_y_y = integral_image_y_->getSum(pos_x-rect_width_/2, pos_y-rect_height_/2, rect_width_, rect_height_, 1);
    const float mean_y_z = integral_image_y_->getSum(pos_x-rect_width_/2, pos_y-rect_height_/2, rect_width_, rect_height_, 2);

    const float normal_x = mean_x_y * mean_y_z - mean_x_z * mean_y_y;
    const float normal_y = mean_x_z * mean_y_x - mean_x_x * mean_y_z;
    const float normal_z = mean_x_x * mean_y_y - mean_x_y * mean_y_x;

    const float normal_length = sqrt(SQR(normal_x) + SQR(normal_y) + SQR(normal_z));
    
    if (normal_length == 0.0f)
    {
      pcl::Normal normal;
      normal.normal_x = 0.0f;
      normal.normal_y = 0.0f;
      normal.normal_z = 0.0f;
      normal.curvature = 0.0f;
      
      return normal;
    }
    
    const float scale = -1.0f/normal_length;
    
    pcl::Normal normal;
    normal.normal_x = normal_x*scale;
    normal.normal_y = normal_y*scale;
    normal.normal_z = normal_z*scale;
    normal.curvature = 0.0f;
    
    return normal;
  }
  //else if (normal_estimation_method_ == AVERAGE_DEPTH_CHANGE)
  //{
  //  pcl::PointXYZ * points = reinterpret_cast<pcl::PointXYZ*>(data_);

  //  pcl::PointXYZ pointL = points[pos_y*width_+pos_x-1];
  //  pcl::PointXYZ pointR = points[pos_y*width_+pos_x+1];
  //  pcl::PointXYZ pointU = points[(pos_y-1)*width_+pos_x];
  //  pcl::PointXYZ pointD = points[(pos_y+1)*width_+pos_x];
  //  
  //  const float mean_x_z = integral_image_x_->getSum(pos_x-rect_width_/2, pos_y-rect_height_/2, rect_width_, rect_height_, 0)/((rect_width_+1)*(rect_height_+1));
  //  const float mean_y_z = integral_image_y_->getSum(pos_x-rect_width_/2, pos_y-rect_height_/2, rect_width_, rect_height_, 0)/((rect_width_+1)*(rect_height_+1));

  //  const float mean_x_x = (pointR.x - pointL.x)/2.0f;
  //  const float mean_x_y = (pointR.y - pointL.y)/2.0f;
  //  const float mean_y_x = (pointD.x - pointU.x)/2.0f;
  //  const float mean_y_y = (pointD.y - pointU.y)/2.0f;

  //  const float normal_x = mean_x_y * mean_y_z - mean_x_z * mean_y_y;
  //  const float normal_y = mean_x_z * mean_y_x - mean_x_x * mean_y_z;
  //  const float normal_z = mean_x_x * mean_y_y - mean_x_y * mean_y_x;

  //  const float normal_length = sqrt(SQR(normal_x) + SQR(normal_y) + SQR(normal_z));
  //  
  //  if (normal_length == 0.0f)
  //  {
  //    pcl::Normal normal;
  //    normal.normal_x = 0.0f;
  //    normal.normal_y = 0.0f;
  //    normal.normal_z = 0.0f;
  //    normal.curvature = 0.0f;
  //    
  //    return normal;
  //  }
  //  
  //  const float scale = -1.0f/normal_length;
  //  
  //  pcl::Normal normal;
  //  normal.normal_x = normal_x*scale;
  //  normal.normal_y = normal_y*scale;
  //  normal.normal_z = normal_z*scale;
  //  normal.curvature = 0.0f;
  //  
  //  return normal;
  //}
  else if (normal_estimation_method_ == AVERAGE_DEPTH_CHANGE)
  {
    pcl::PointXYZ * points = reinterpret_cast<pcl::PointXYZ*>(data_);

    pcl::PointXYZ pointL = points[pos_y*width_+pos_x-rect_width_/2];
    pcl::PointXYZ pointR = points[pos_y*width_+pos_x+rect_width_/2];
    pcl::PointXYZ pointU = points[(pos_y-rect_height_/2)*width_+pos_x];
    pcl::PointXYZ pointD = points[(pos_y+rect_height_/2)*width_+pos_x];

    const float mean_L_z = integral_image_->getSum(pos_x-1-rect_width_/2, pos_y-rect_height_/2, rect_width_-1, rect_height_-1, 0)/((rect_width_-1)*(rect_height_-1));
    const float mean_R_z = integral_image_->getSum(pos_x+1-rect_width_/2, pos_y-rect_height_/2, rect_width_-1, rect_height_-1, 0)/((rect_width_-1)*(rect_height_-1));
    const float mean_U_z = integral_image_->getSum(pos_x-rect_width_/2, pos_y-1-rect_height_/2, rect_width_-1, rect_height_-1, 0)/((rect_width_-1)*(rect_height_-1));
    const float mean_D_z = integral_image_->getSum(pos_x-rect_width_/2, pos_y+1-rect_height_/2, rect_width_-1, rect_height_-1, 0)/((rect_width_-1)*(rect_height_-1));

    const float mean_x_z = (mean_R_z - mean_L_z)/2.0f;
    const float mean_y_z = (mean_D_z - mean_U_z)/2.0f;

    const float mean_x_x = (pointR.x - pointL.x)/(rect_width_);
    const float mean_x_y = (pointR.y - pointL.y)/(rect_height_);
    const float mean_y_x = (pointD.x - pointU.x)/(rect_width_);
    const float mean_y_y = (pointD.y - pointU.y)/(rect_height_);

    const float normal_x = mean_x_y * mean_y_z - mean_x_z * mean_y_y;
    const float normal_y = mean_x_z * mean_y_x - mean_x_x * mean_y_z;
    const float normal_z = mean_x_x * mean_y_y - mean_x_y * mean_y_x;

    const float normal_length = sqrt(SQR(normal_x) + SQR(normal_y) + SQR(normal_z));
    
    if (normal_length == 0.0f)
    {
      pcl::Normal normal;
      normal.normal_x = 0.0f;
      normal.normal_y = 0.0f;
      normal.normal_z = 0.0f;
      normal.curvature = 0.0f;
      
      return normal;
    }
    
    const float scale = -1.0f/normal_length;
    
    pcl::Normal normal;
    normal.normal_x = normal_x*scale;
    normal.normal_y = normal_y*scale;
    normal.normal_z = normal_z*scale;
    normal.curvature = 0.0f;
    
    return normal;
  }

  pcl::Normal normal;
  normal.normal_x = 0.0f;
  normal.normal_y = 0.0f;
  normal.normal_z = 0.0f;
  normal.curvature = 0.0f;
      
  return normal;
}

