#ifndef RIGTFORM_H
#define RIGTFORM_H

#include <cassert>
#include <iostream>

#include "matrix4.h"
#include "quat.h"

class RigTForm {
  Cvec3 t_; // translation component
  Quat r_;  // rotation component represented as a quaternion

public:
  RigTForm() : t_(0) { assert(norm2(Quat(1, 0, 0, 0) - r_) < CS175_EPS2); }

  // constructor that initialzes t_ (Translation) and r_ (Rotation).
  RigTForm(const Cvec3 &t, const Quat &r) : t_(t), r_(r){};

  // constructor that initialzes t_ (Translation) only.
  // explicit keyword prevents implicit conversions.
  explicit RigTForm(const Cvec3 &t) : t_(t), r_(){};

  // constructor that initialzes r_ (Rotation) only.
  // explicit keyword prevents implicit conversions.
  explicit RigTForm(const Quat &r) : t_(), r_(r){};

  Cvec3 getTranslation() const { return t_; }

  Quat getRotation() const { return r_; }

  RigTForm &setTranslation(const Cvec3 &t) {
    t_ = t;
    return *this;
  }

  RigTForm &setRotation(const Quat &r) {
    r_ = r;
    return *this;
  }

  // The product of a RigTForm and a Cvec4
  // Returns A.r * a + A.t
  Cvec4 operator*(const Cvec4 &a) const {
    return getRotation() * a + Cvec4(getTranslation(), 0);
  }

  // The product of two RigTForm
  // Returns
  // [i t1+r1t2] * [r1r2 0]
  // [0       1]   [0    1]
  RigTForm operator*(const RigTForm &a) const {
    // The transformation part is t1+r1t2
    Cvec3 transformation =
        getTranslation() + Cvec3(getRotation() * Cvec4(a.getTranslation(), 0));

    // The rotation part is r1r2
    Quat rotation = getRotation() * a.getRotation();

    return RigTForm(transformation, rotation);
  }
};

// The inverse of a RigTForm
// Returns
// [i -r^-1*t] * [r^-1 0]
// [0       1]   [0    1]
inline RigTForm inv(const RigTForm &tform) {
  // The transformation part is -r^-1*t
  Cvec3 transformation =
      -Cvec3(inv(tform.getRotation()) * Cvec4(tform.getTranslation(), 0));

  // The rotation part is r^-1
  Quat rotation = inv(tform.getRotation());

  return RigTForm(transformation, rotation);
}

inline RigTForm transFact(const RigTForm &tform) {
  return RigTForm(tform.getTranslation());
}

inline RigTForm linFact(const RigTForm &tform) {
  return RigTForm(tform.getRotation());
}

// Transforms a RigTForm into a 4x4 Matrix
// Returns Matrix4 representing the RigTform object.
inline Matrix4 rigTFormToMatrix(const RigTForm &tform) {
  Matrix4 T = Matrix4::makeTranslation(tform.getTranslation());
  Matrix4 R = quatToMatrix(tform.getRotation());
  return T * R;
}

#endif
