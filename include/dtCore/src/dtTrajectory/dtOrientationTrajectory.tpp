namespace dtCore {

template <typename ValueType>
dtOrientationTrajectory<ValueType>::dtOrientationTrajectory() {}

template <typename ValueType>
dtOrientationTrajectory<ValueType>::dtOrientationTrajectory(
    const ValueType t0, const ValueType tf, const ContRefType initial,
    const ContRefType final) {}

template <typename ValueType>
dtOrientationTrajectory<ValueType>::~dtOrientationTrajectory() {}

template <typename ValueType>
void dtOrientationTrajectory<ValueType>::Interpolate(
    const ValueType t, ContRefType current) const {}

template <typename ValueType>
void dtOrientationTrajectory<ValueType>::Reconfigure(const ContRefType initial,
                                                     const ContRefType final) {}

} // namespace dtCore