asmlinkage int sys_set_mlimit(uid_t uid, long mem_max){
	return set_mlimit(uid, mem_max);
}