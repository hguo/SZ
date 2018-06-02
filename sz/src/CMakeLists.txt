set (sz_sources
  ByteToolkit.c
  CompressElement.c
  DynamicByteArray.c
  DynamicDoubleArray.c
  DynamicFloatArray.c
  DynamicIntArray.c
  Huffman.c
  TightDataPointStorageD.c
  TightDataPointStorageF.c
  TightDataPointStorageI.c
  TypeManager.c
  VarSet.c
  callZlib.c
  conf.c
  dataCompression.c
  dictionary.c
  iniparser.c
  pastri.c
  rw.c
  rwf.c
  sz.c
  sz_double.c
  sz_double_pwr.c
  sz_double_ts.c
  sz_float.c
  sz_float_pwr.c
  sz_float_ts.c
  sz_int16.c
  sz_int32.c
  sz_int64.c
  sz_int8.c
  # sz_omp.c
  # sz_omp_tmp.c
  sz_uint16.c
  sz_uint32.c
  sz_uint64.c
  sz_uint8.c
  szd_double.c
  szd_double_pwr.c
  szd_double_ts.c
  szd_float.c
  szd_float_pwr.c
  szd_float_ts.c
  szd_int16.c
  szd_int32.c
  szd_int64.c
  szd_int8.c
  szd_uint16.c
  szd_uint32.c
  szd_uint64.c
  szd_uint8.c
  # szf.c
)

add_library (SZ ${sz_sources})
target_link_libraries (SZ ${ZLIB_LIBRARIES})
