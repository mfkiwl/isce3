#!/usr/bin/env python3 #
# Author: Liang Yu
# Copyright 2019-

def runPrepHDF5(self, userconfig, defaults=None):
    '''
    Copies shared data from RSLC HDF5 to GSLC HDF5

    Parameters:
    -----------
    path_src : str
        Full path to source HDF5 file
    path_dst : str
        Full path to destination HDF5 file
    '''

    import h5py
    import os
    from nisar.h5 import cp_h5_meta_data

    # prelim setup
    common_parent_path = 'science/LSAR'
    src_h5 = h5py.File(userconfig['inputs']['rslc'], 'r')

    # rm anything and start from scratch
    try:
        os.remove(userconfig['outputs']['gslc'])
    except FileNotFoundError:
        pass

    dst_h5 = h5py.File(userconfig['outputs']['gslc'], 'w')

    # simple copies of identification, metadata/orbit, metadata/attitude groups
    cp_h5_meta_data(src_h5, dst_h5, os.path.join(common_parent_path, 'identification'))
    cp_h5_meta_data(src_h5, dst_h5, os.path.join(common_parent_path, 'SLC/metadata/orbit'))
    cp_h5_meta_data(src_h5, dst_h5, os.path.join(common_parent_path, 'SLC/metadata/attitude'))

    # copy calibration information group
    cp_h5_meta_data(src_h5, dst_h5,
            os.path.join(common_parent_path, 'SLC/metadata/calibrationInformation'),
            os.path.join(common_parent_path, 'GSLC/metadata/calibrationInformation'),
            excludes=['zeroDopplerTime', 'slantRange'])
                
    # copy processing information group
    cp_h5_meta_data(src_h5, dst_h5,
            os.path.join(common_parent_path, 'SLC/metadata/processingInformation'),
            os.path.join(common_parent_path, 'GSLC/metadata/processingInformation'),
            excludes=['l0bGranules', 'demFiles', 'zeroDopplerTime', 'slantRange'])

    # copy radar grid information group
    cp_h5_meta_data(src_h5, dst_h5,
            os.path.join(common_parent_path, 'SLC/metadata/geolocationGrid'),
            os.path.join(common_parent_path, 'GSLC/metadata/radarGrid'),
            renames={'coordinateX':'xCoordinates',
                'coordinateY':'yCoordinates',
                'zeroDopplerTime':'zeroDopplerAzimuthTime'})

    # copy radar imagery group; assumming shared data
    # XXX option0: to be replaced with actual gcov code
    # XXX option1: do not write GSLC data here; GSLC rasters can be appended to the GSLC HDF5
    for freq in ['A', 'B']:
        ds_ref = os.path.join(common_parent_path, f'SLC/swaths/frequency{freq}')
        if ds_ref not in src_h5:
            continue
        cp_h5_meta_data(src_h5, dst_h5,
                ds_ref,
                os.path.join(common_parent_path, f'GSLC/grids/frequency{freq}'),
                excludes=['acquiredCenterFrequency', 'acquiredAzimuthBandwidth', 
                    'acquiredRangeBandwidth', 'nominalAcquisitionPRF', 'slantRange',
                    'sceneCenterAlongTrackSpacing', 'sceneCenterGroundRangeSpacing',
                    'HH', 'HV', 'VH', 'VV', 'RH', 'RV',
                    'validSamplesSubSwath1', 'validSamplesSubSwath2',
                    'validSamplesSubSwath3', 'validSamplesSubSwath4'],
                renames={'processedCenterFrequency':'centerFrequency',
                    'processedAzimuthBandwidth':'azimuthBandwidth',
                    'processedRangeBandwidth':'rangeBandwidth'})

# end of file