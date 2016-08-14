#include <linux/module.h>       /* Needed by all modules */
#include <linux/kernel.h>       /* Needed for KERN_INFO */
#include <linux/init.h>
#include <linux/interrupt.h>
#include <linux/of.h>
#include <linux/slab.h>
#include <linux/sysfs.h>
#include <linux/fs.h>
#include <linux/vmalloc.h>
#include <linux/fcntl.h>
#include <linux/kobject.h>
#include <asm/uaccess.h>

struct elog_obj {
        struct kobject kobj;
        struct bin_attribute raw_attr;
        uint64_t id;
        uint64_t type;
        size_t size;
        char *buffer;
};
#define to_elog_obj(x) container_of(x, struct elog_obj, kobj)

struct elog_attribute {
        struct attribute attr;
        ssize_t (*show)(struct elog_obj *elog, struct elog_attribute *attr,
                        char *buf);
        ssize_t (*store)(struct elog_obj *elog, struct elog_attribute *attr,
                         const char *buf, size_t count);
};
#define to_elog_attr(x) container_of(x, struct elog_attribute, attr)

static ssize_t elog_id_show(struct elog_obj *elog_obj,
                            struct elog_attribute *attr,
                            char *buf)
{
        return sprintf(buf, "0x%llx\n", elog_obj->id);
}

static const char *elog_type_to_string(uint64_t type)
{
        switch (type) {
        case 0: return "PEL";
        default: return "unknown";
        }
}

static ssize_t elog_type_show(struct elog_obj *elog_obj,
                              struct elog_attribute *attr,
                              char *buf)
{
        return sprintf(buf, "0x%llx %s\n",
                       elog_obj->type,
                       elog_type_to_string(elog_obj->type));
}

static ssize_t elog_ack_show(struct elog_obj *elog_obj,
                             struct elog_attribute *attr,
                             char *buf)
{
        return sprintf(buf, "ack - acknowledge log message\n");
}

static ssize_t elog_ack_store(struct elog_obj *elog_obj,
                              struct elog_attribute *attr,
                              const char *buf,
                              size_t count)
{
	sysfs_remove_file_self(&elog_obj->kobj, &attr->attr);
        kobject_put(&elog_obj->kobj);
        return count;
}

static struct elog_attribute id_attribute =
        __ATTR(id, S_IRUGO, elog_id_show, NULL);
static struct elog_attribute type_attribute =
        __ATTR(type, S_IRUGO, elog_type_show, NULL);
static struct elog_attribute ack_attribute =
        __ATTR(acknowledge, 0660, elog_ack_show, elog_ack_store);

static struct kset *elog_kset;

static ssize_t elog_attr_show(struct kobject *kobj,
                              struct attribute *attr,
                              char *buf)
{
        struct elog_attribute *attribute;
        struct elog_obj *elog;

        attribute = to_elog_attr(attr);
        elog = to_elog_obj(kobj);

        if (!attribute->show)
                return -EIO;

        return attribute->show(elog, attribute, buf);
}

static ssize_t elog_attr_store(struct kobject *kobj,
                               struct attribute *attr,
                               const char *buf, size_t len)
{
        struct elog_attribute *attribute;
        struct elog_obj *elog;

        attribute = to_elog_attr(attr);
        elog = to_elog_obj(kobj);

        if (!attribute->store)
                return -EIO;

        return attribute->store(elog, attribute, buf, len);
}

static const struct sysfs_ops elog_sysfs_ops = {
        .show = elog_attr_show,
        .store = elog_attr_store,
};

static void elog_release(struct kobject *kobj)
{
        struct elog_obj *elog;

        elog = to_elog_obj(kobj);
        kfree(elog->buffer);
        kfree(elog);
}

static struct attribute *elog_default_attrs[] = {
        &id_attribute.attr,
        &type_attribute.attr,
        &ack_attribute.attr,
        NULL,
};

static struct kobj_type elog_ktype = {
        .sysfs_ops = &elog_sysfs_ops,
        .release = &elog_release,
        .default_attrs = elog_default_attrs,
};


//int __init opal_elog_init(void)
int init_module(void)
{

         struct elog_obj *elog;
         int rc;

        printk(KERN_INFO "Hello world mukesh.\n");
        elog_kset = kset_create_and_add("elog", NULL, firmware_kobj);
        if (!elog_kset) {
                pr_warn("%s: failed to create elog kset\n", __func__);
                return -1;
        }

         elog = kzalloc(sizeof(*elog), GFP_KERNEL);
         if (!elog)
                return -1;

        elog->kobj.kset = elog_kset;
 
        kobject_init(&elog->kobj, &elog_ktype);
         rc = kobject_add(&elog->kobj, NULL, "%s", "Mukesh");
         if (rc) {
                 kobject_put(&elog->kobj);
                 return -1;
         }

        return 0;
}

void cleanup_module(void)
{
        printk(KERN_INFO "Goodbye world 1.\n");
	kobject_put(&elog_kset->kobj);
}


MODULE_LICENSE("GPL");
    
