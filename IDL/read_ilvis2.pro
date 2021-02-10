PRO read_ilvis2, data, header, file=file

; IDL code to read in LVIS Level 2 data that is in text format (.TXT extension) - 2013/9/26
; INPUT:
;    file: file to read in; if this parameter is not present, a popup box 
;    will appear and offer to select a file with a .TXT extension
; OUTPUT:
;    data: data returned as a structure (LVIS_LEVEL2 structure defined below)
;    header: header lines returned (text)
; Problems? Contact us at http://lvis.gsfc.nasa.gov/

; structure for LVIS Level 2 data
lvis_level2_data = {LVIS_LEVEL2, $
   lvis_lfid:          0ul,  $ ; LVIS file identification
   shotnumber:         0ul,  $ ; Laser shot assigned during collection
   time:               0.0d, $ ; UTC decimal seconds of the day
   longitude_centroid: 0.0d, $ ; Centroid longitude of the corresponding Level-1B waveform
   latitude_centroid:  0.0d, $ ; Centroid latitude of the corresponding Level-1B waveform
   elevation_centroid: 0.0d, $ ; Centroid elevation of the corresponding Level-1B waveform
   longitude_low:      0.0d, $ ; Longitude of the lowest detected mode within the waveform
   latitude_low:       0.0d, $ ; Latitude of the lowest detected mode within the waveform
   elevation_low:      0.0d, $ ; Mean elevation of the lowest detected mode within the waveform
   longitude_high:     0.0d, $ ; Longitude of the center of the highest mode in the waveform
   latitude_high:      0.0d, $ ; Latitude of the center of the highest mode in the waveform
   elevation_high:     0.0d}   ; Elevation of the center of the highest mode in the waveform

; select input file
if keyword_set(file) then begin
   if (file_test(file) eq 1) then begin
      infile = file
   endif else begin
      infile = dialog_pickfile(/read, filter='*.TXT')
   endelse
endif else begin
   infile = dialog_pickfile(/read, filter='*.TXT')
endelse

print, 'Reading in LVIS LEVEL2 file: ', infile

nline = file_lines(infile)
ndata = nline

strline=''
lflag=0

openr, lun, infile, /get_lun

; count the number of header lines
while ((lflag eq 0) and (eof(lun) eq 0)) do begin
   readf, lun, strline
   test = byte(strmid(strline, 0, 1)) ; is this a number?
   if (test ge 48 and test le 57) then lflag = 1 $ ; yes
      else ndata = ndata - 1L                      ; no
endwhile

close, lun
openr, lun, infile

; skip header and read in the data
if (lflag eq 1) then begin

   nhdr = nline - ndata
   if (nhdr gt 0) then begin
      header = strarr(nhdr)
      readf, lun, header
   endif
   data = replicate(lvis_level2_data, ndata)
   readf, lun, data

endif else begin
   print, 'Error reading in file ', infile
endelse

close, lun
free_lun, lun


END
