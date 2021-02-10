PRO read_ilvis1b, data, file=file, records=records

; IDL code to read in LVIS Level 1B Version 4 data (.LGW4 extension) - 2013/9/26
; and                 LVIS Level 1B Version 5 data (.h5 extension) - 2014/6/10
; INPUT:
;    file: file to read in; if this parameter is not present, a popup box
;       will appear and offer to select a file with an .h5 or .LGW4 extension
;    records=N :  will only read in N records
; OUTPUT:
;    data: data returned as a structure (LVIS_LEVEL1B structure defined below)
; Problems? Contact us at http://lvis.gsfc.nasa.gov/

lvis_level1b_data = { LVIS_LEVEL1B, $
   lvis_lfid:     0UL,  $ ; LVIS file identification
   shotnumber:    0UL,  $ ; Laser shot assigned during collection
   azimuth:       0.0,  $ ; Azimuth angle of laser beam
   incidentangle: 0.0,  $ ; Off-nadir angle of laser beam
   range:         0.0,  $ ; Along-laser-beam distance from the instrument to the ground
   time:          0.0D, $ ; UTC decimal seconds of the day
   lon_0:         0.0D, $ ; Longitude of the highest sample in the waveform
   lat_0:         0.0D, $ ; Latitude of the highest sample in the waveform
   z_0:           0.0,  $ ; Elevation of the highest sample in the waveform
   lon_527:       0.0D, $ ; Longitude of the lowest sample in the waveform
   lat_527:       0.0D, $ ; Latitude of the lowest sample in the waveform
   z_527:         0.0,  $ ; Elevation of the lowest sample in the waveform
   sigmean:       0.0,  $ ; Signal mean noise level
   txwave: uintarr(120),$ ; Transmitted waveform (120 samples, 2 bytes per sample) 
   rxwave: uintarr(528) } ; Received waveform (528 samples, 2 bytes per sample)

; select input file
if keyword_set(file) then begin
   if (file_test(file) eq 1) then begin
      infile = file
   endif else begin
      infile = dialog_pickfile(/read, filter=['*.h5','*.LGW4'])
   endelse
endif else begin
   infile = dialog_pickfile(/read, filter=['*.h5','*.LGW4'])
endelse

file_ext=strmid(infile, 1, 2, /reverse_offset)

if (file_ext ne 'h5' and file_ext ne 'W4') then begin
   print,'The file extension must be *.h5 or *.LGW4: ', infile
   stop
endif

print, 'Reading in LVIS LEVEL-1B file: ', infile

if (file_ext eq 'h5') then begin

   print,'Format: HDF5'
   hdf = h5_parse(infile, /read_data)
   numshots = hdf.lfid._nelements
   if (numshots le 0) then begin
      print, 'No shots found: hdf.lfid._nelements = ', numshots
      STOP
   endif

   data = replicate(lvis_level1b_data, hdf.lfid._nelements)

   data.lvis_lfid     = hdf.lfid._data
   data.shotnumber    = hdf.shotnumber._data
   data.azimuth       = hdf.azimuth._data
   data.incidentangle = hdf.incidentangle._data
   data.range         = hdf.range._data
   data.time          = hdf.time._data
   data.lon_0         = hdf.lon0._data
   data.lat_0         = hdf.lat0._data
   data.z_0           = hdf.z0._data
   data.lon_527       = hdf.lon527._data
   data.lat_527       = hdf.lat527._data
   data.z_527         = hdf.z527._data
   data.sigmean       = hdf.sigmean._data
   data.txwave        = hdf.txwave._data
   data.rxwave        = hdf.rxwave._data

   if(keyword_set(RECORDS)) then data = data[0:records-1]

endif else begin

   print,'Format: LGW4'

   ; figure out number of records contained in file:
   openr, lun, infile, /get_lun, /SWAP_IF_LITTLE_ENDIAN
   stats = fstat(lun)

   recsize = n_tags(lvis_level1b_data, /data_length)

   test = stats.size mod recsize
   if (test ne 0) then begin
      print, 'Error reading in file ', infile
      stop
   endif

   numshots = stats.size/recsize
   if(keyword_set(RECORDS)) then numshots = long(records)

   if (numshots gt 0) then begin
      data = replicate(lvis_level1b_data, numshots)
      readu, lun, data
   endif

   close, lun
   free_lun, lun

endelse


END
