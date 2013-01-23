#include "pose.h"

namespace {

void getCorrespondencesForInstance(
    const vobj_frame *frame_, const vobj_instance *instance,
    std::vector<cv::Point3f> *object_points,
    std::vector<cv::Point2f> *projections) {
  object_points->clear();
  projections->clear();

  // vobj_frame does not have const_iterator yet. We have to const_cast :(
  vobj_frame *frame = const_cast<vobj_frame *>(frame_);
  for (tracks::keypoint_frame_iterator it(frame->points.begin());
       !it.end(); ++it) {
    const vobj_keypoint *kpt = static_cast<vobj_keypoint *>(it.elem());
    if (kpt->vobj != instance->object || kpt->obj_kpt == 0) {
      continue;
    }

    object_points->push_back(cv::Point3f(
            kpt->obj_kpt->u,
            kpt->obj_kpt->v,
            0));
    projections->push_back(cv::Point2f(
            kpt->u,
            kpt->v));
  }
}

}  // namespace

void computeObjectPose(const vobj_frame *frame, const vobj_instance *instance,
                       PerspectiveCamera *camera) {
  std::vector<cv::Point3f> object_points;
  std::vector<cv::Point2f> projections;

  getCorrespondencesForInstance(frame, instance, &object_points, &projections);

  camera->setPoseFromHomography(instance->transform);

  if (projections.size() > 4) {
	  cv::Mat intrinsics = camera->getIntrinsics();
	  cv::Mat rotation = camera->getExpMapRotation();
	  cv::Mat translation = camera->getTranslation();
	  cv::solvePnP(object_points, projections, intrinsics, camera->distortion,
				   rotation, translation,
				   true, // use extrinsic guess
				   CV_ITERATIVE);

	  camera->setExpMapRotation(rotation);
	  camera->setTranslation(translation);
  }
}

PoseFilter::PoseFilter(float alpha) : alpha_(alpha) {
	assert(alpha > 0.0f);
	assert(alpha <= 1.0f);
}

void PoseFilter::update(PerspectiveCamera *camera) {
	cv::addWeighted(rotation_, 1.0 - alpha_, camera->getExpMapRotation(), alpha_, 0, rotation_);
	camera->setExpMapRotation(rotation_);

	cv::addWeighted(translation_, 1.0 - alpha_, camera->getTranslation(), alpha_, 0, translation_);
	camera->setTranslation(translation_);
}

void PoseFilter::reset(const PerspectiveCamera *camera) {
	rotation_ = camera->getExpMapRotation();
	translation_ = camera->getTranslation();
}
