
kd_init struc

  kdebug_extension                    dd    0
  io_apic_ptr                         dd    0
  local_apic_ptr                      dd    0
  physical_kernel_info_page_ptr       dd    0
  kernel_proot_ptr_ptr                dd    0
  kdebug_configuration                dd    0
  kdebug_permissions                  dd    0
  irq0_intr_number                    dd    0
  grab_frame_proc                     dd    0

kd_init ends


 