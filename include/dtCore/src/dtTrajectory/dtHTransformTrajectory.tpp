namespace dtCore {

template <typename ValueType>
dtHTransformTrajectory<ValueType>::dtHTransformTrajectory() {}

template <typename ValueType>
dtHTransformTrajectory<ValueType>::dtHTransformTrajectory(
    const double t0, const double tf, const ContainerType &initial,
    const ContainerType &final) {}

template <typename ValueType>
dtHTransformTrajectory<ValueType>::~dtHTransformTrajectory() {}

template <typename ValueType>
void dtHTransformTrajectory<ValueType>::interpolate(
    const double t, ContainerType &current) const {}

template <typename ValueType>
void dtHTransformTrajectory<ValueType>::_determineCoeff(
    const ContainerType &initial, const ContainerType &final) {}

} // namespace dtCore