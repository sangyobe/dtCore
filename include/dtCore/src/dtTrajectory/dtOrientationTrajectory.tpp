namespace dtCore {

template <typename ValueType>
dtOrientationTrajectory<ValueType>::dtOrientationTrajectory() {}

template <typename ValueType>
dtOrientationTrajectory<ValueType>::dtOrientationTrajectory(
    const double t0, const double tf, const ContainerType &initial,
    const ContainerType &final) {}

template <typename ValueType>
dtOrientationTrajectory<ValueType>::~dtOrientationTrajectory() {}

template <typename ValueType>
void dtOrientationTrajectory<ValueType>::interpolate(
    const double t, ContainerType &current) const {}

template <typename ValueType>
void dtOrientationTrajectory<ValueType>::_determineCoeff(
    const ContainerType &initial, const ContainerType &final) {}

} // namespace dtCore