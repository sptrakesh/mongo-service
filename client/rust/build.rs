fn main() {
  if cfg!(target_os = "macos")
  {
    cxx_build::bridge("src/client.rs")
        .cpp(true)
        .include("/usr/local/spt/include")
        .include("/usr/local/boost/include")
        .include("/usr/local/mongo/include")
        .include("/usr/local/mongo/include/bsoncxx/v_noabi")
        .file("src/mongo-service.cpp")
        .std("c++23")
        .compile("mongo-service");

    println!("cargo::rustc-link-search=/usr/local/spt/lib");
    println!("cargo::rustc-link-search=/usr/local/mongo/lib");
  }
  else
  {
    cxx_build::bridge("src/client.rs")
        .cpp(true)
        .include("/opt/spt/include")
        .include("/opt/local/include")
        .file("src/mongo-service.cpp")
        .std("c++23")
        .compile("mongo-service");
    println!("cargo::rustc-link-search=/opt/spt/lib");
  }

  if cfg!(target_os = "linux") { println!("cargo::rustc-link-lib=gcc_eh"); }

  println!("cargo::rustc-link-lib=bson2");
  println!("cargo::rustc-link-lib=bsoncxx-static");
  println!("cargo::rustc-link-lib=nanolog");
  println!("cargo::rustc-link-lib=mongoservicecommon");
  println!("cargo::rustc-link-lib=mongoserviceapi");
  println!("cargo:rerun-if-changed=src/mongo-service.cpp");
  println!("cargo:rerun-if-changed=include/mongo-service.hpp");
}