#include <linux/build-salt.h>
#include <linux/module.h>
#include <linux/vermagic.h>
#include <linux/compiler.h>

BUILD_SALT;

MODULE_INFO(vermagic, VERMAGIC_STRING);
MODULE_INFO(name, KBUILD_MODNAME);

__visible struct module __this_module
__attribute__((section(".gnu.linkonce.this_module"))) = {
	.name = KBUILD_MODNAME,
	.init = init_module,
#ifdef CONFIG_MODULE_UNLOAD
	.exit = cleanup_module,
#endif
	.arch = MODULE_ARCH_INIT,
};

#ifdef CONFIG_RETPOLINE
MODULE_INFO(retpoline, "Y");
#endif

static const struct modversion_info ____versions[]
__used
__attribute__((section("__versions"))) = {
	{ 0xa16cf51b, "module_layout" },
	{ 0x5cb4d29e, "cdev_del" },
	{ 0x2b68bd2f, "del_timer" },
	{ 0x6091b333, "unregister_chrdev_region" },
	{ 0x6f5a2d80, "cdev_add" },
	{ 0x25337c5f, "cdev_init" },
	{ 0x3fd78f3b, "register_chrdev_region" },
	{ 0x28cc25db, "arm_copy_from_user" },
	{ 0xdb7305a1, "__stack_chk_fail" },
	{ 0x11a13e31, "_kstrtol" },
	{ 0x85df9b6c, "strsep" },
	{ 0x8f678b07, "__stack_chk_guard" },
	{ 0xe2d5255a, "strcmp" },
	{ 0xc38c83b8, "mod_timer" },
	{ 0xe914e41e, "strcpy" },
	{ 0xcf8d2e6f, "kmem_cache_alloc_trace" },
	{ 0xb44e414c, "kmalloc_caches" },
	{ 0xf4fa543b, "arm_copy_to_user" },
	{ 0x88db9f48, "__check_object_size" },
	{ 0x91715312, "sprintf" },
	{ 0x97255bdf, "strlen" },
	{ 0x526c3a6c, "jiffies" },
	{ 0xc6f46339, "init_timer_key" },
	{ 0x12da5bb2, "__kmalloc" },
	{ 0x2e5810c6, "__aeabi_unwind_cpp_pr1" },
	{ 0x37a0cba, "kfree" },
	{ 0x7c32d0f0, "printk" },
	{ 0xb1ad28e0, "__gnu_mcount_nc" },
};

static const char __module_depends[]
__used
__attribute__((section(".modinfo"))) =
"depends=";

