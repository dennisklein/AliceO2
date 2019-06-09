o2_add_library(TPCFastTransformation
               SOURCES IrregularSpline1D.cxx
                       IrregularSpline2D3D.cxx
                       SemiregularSpline2D3D.cxx
                       IrregularSpline2D3DCalibrator.cxx
                       IrregularSpline2D3DCalibrator.cxx
                       TPCDistortionIRS.cxx
                       TPCFastTransform.cxx
                       TPCFastTransformGeo.cxx
               PUBLIC_INCLUDE_DIRECTORIES ${CMAKE_CURRENT_LIST_DIR}
               PUBLIC_LINK_LIBRARIES O2::GPUCommon Vc::Vc ROOT::Core)

o2_target_root_dictionary(TPCFastTransformation
                          HEADERS IrregularSpline1D.h
                                  IrregularSpline2D3D.h
                                  IrregularSpline2D3DCalibrator.h
                                  TPCFastTransform.h
                                  TPCDistortionIRS.h
                                  SemiregularSpline2D3D.h
                                  IrregularSpline2D3DCalibrator.h
                                  RegularSpline1D.h
                                  TPCFastTransformGeo.h
                          LINKDEF TPCFastTransformationLinkDef_O2.h)
