use crate::client::cpp::Pool;

#[cxx::bridge]
pub mod cpp
{
  /// Represents the log level for the C++ API logger.
  enum LogLevel
  {
    /// Critical log level.
    CRIT,
    /// Warning log level.
    WARN,
    /// Informational log level.
    INFO,
    /// Debug log level.
    DEBUG
  }

  /// A structure representing a logger's configuration and properties.
  ///
  /// The `Logger` struct provides a way to encapsulate logging-related
  /// settings, such as the destination path for logs, the logger's name,
  /// the logging level, and whether to log messages to the console.
  ///
  /// # Fields
  ///
  /// * `path` - A `String` representing the file path where log outputs will
  ///   be stored. If logging to files is not required, this can be empty or
  ///   unused based on implementation.
  /// * `name` - A `String` containing the name or identifier of the logger.
  ///   This could represent the application, module, or context the logger
  ///   is associated with.
  /// * `level` - A `LogLevel` enum defining the severity level of logs
  ///   (e.g., DEBUG, INFO, WARN, ERROR). This dictates what type of messages
  ///   the logger will handle.
  /// * `console` - A `bool` indicating whether log messages should
  ///   also be output to the console. If `true`, log entries are printed
  ///   to the console; otherwise, they are not.
  ///
  /// # Example
  ///
  /// ```rust
  /// use mongo_service::{Logger, client::cpp::LogLevel};
  /// let logger = Logger {
  ///     path: String::from("/var/log/app.log"),
  ///     name: String::from("AppLogger"),
  ///     level: LogLevel::INFO,
  ///     console: true,
  /// };
  /// ```
  struct Logger
  {
    /// The path to the directory under which log files are stored.  *Must* end with a trailing slash (`/`).
    path: String,
    /// The prefix name for the log file.
    name: String,
    /// The log level for the logger.  This determines the severity of log messages that will be handled.
    level: LogLevel,
    /// Whether to log messages to the console.  If `true`, log messages will be printed to the console; otherwise, they will not.
    console: bool
  }

  /// A structure representing the configuration of a connection pool.
  ///
  /// Members:
  /// - `initialSize` (`u32`):
  ///   The number of connections that are initially created when the pool is initialised.
  ///   This is the starting size of the connection pool.
  ///
  /// - `maxPoolSize` (`u32`):
  ///   The maximum number of connections that can exist in the pool at any given time.
  ///   This defines the upper limit of the pool size to prevent overallocation of resources.
  ///
  /// - `maxConnections` (`u32`):
  ///   The maximum number of simultaneous connections that can be in use at any point.
  ///   This value determines how many connections will be available for client use concurrently.
  ///   Connections in excess of `maxPoolSize` will be created and destroyed as needed until this limit is reached.
  ///
  /// - `maxIdleTimeSeconds` (`u32`):
  ///   The maximum duration (in seconds) that a connection can remain idle before it is closed.
  ///   This helps in freeing up unused connections and managing resource utilisation efficiently.
  ///
  /// This structure can be used to configure a flexible and efficient connection pool for
  /// database connections.
  struct Pool
  {
    /// The initial size of the connection pool.
    initialSize: u32,
    /// The maximum number of connections that can exist in the pool at any given time.
    maxPoolSize: u32,
    /// The maximum number of simultaneous connections that can be in use at any point.
    maxConnections: u32,
    /// The maximum duration (in seconds) that a connection can remain idle before it is closed.
    maxIdleTimeSeconds: u32
  }

  /// Represents the configuration settings required for initialising
  /// the C++ API. This struct encapsulates important
  /// details such as the connection pool, application name, host, and port.
  ///
  /// # Fields
  ///
  /// * `pool` - An instance of `Pool` used for managing database or resource connections.
  /// * `application` - A `String` representing the name of the application.
  /// * `host` - A `String` specifying the hostname or IP address where the application will run.
  /// * `port` - A `u16` representing the port number on which the application will listen.
  ///
  /// # Example
  ///
  /// ```rust
  /// use mongo_service::{Configuration, client::cpp::Pool};
  /// let config = Configuration {
  ///     pool: Pool::new(),
  ///     application: String::from("MyApp"),
  ///     host: String::from("127.0.0.1"),
  ///     port: 2000,
  /// };
  /// ```
  struct Configuration
  {
    /// The connection pool configuration.
    pool: Pool,
    /// The name of the application.
    application: String,
    /// The hostname or IP address where the mongo-service is running.
    host: String,
    /// The port number on which the mongo-service is listening.
    port: u16
  }

  unsafe extern "C++"
  {
    include!("mongo-service/include/mongo-service.hpp");

    /// Initializes the logger with the given configuration.
    ///
    /// This function sets up the logger based on the provided `Logger` configuration.
    /// It prepares the logging system to start capturing and outputting logs as per the
    /// provided settings. This is typically one of the first methods to be called in an
    /// application to ensure proper logging functionality.
    ///
    /// # Parameters
    ///
    /// * `conf` - A `Logger` instance containing the desired logging configuration
    ///   such as log level, output format, and destination.
    ///
    /// # Panics
    ///
    /// This function may panic if the logger has already been initialised or if
    /// the provided configuration contains invalid settings.
    ///
    /// # Notes
    ///
    /// Ensure that this function is called only once during the application lifecycle.
    /// Attempting to initialise the logger multiple times may cause unexpected behaviour
    /// or runtime issues.
    pub fn init_logger(conf: Logger);

    /// Initialises the C++ API with the provided configuration.
    ///
    /// # Arguments
    ///
    /// * `conf` - A `Configuration` structure containing the required settings for initialising the C++ API.
    ///
    /// # Notes
    ///
    /// - Ensure that the `Configuration` provided is valid and complete before
    ///   calling this function.
    /// - Calling this function more than once can lead to undefined behaviour.
    ///
    /// # Panics
    ///
    /// This function may panic if the provided configuration is invalid or if
    /// initialisation fails due to system-level errors.
    pub fn init(conf: Configuration);

    /// Executes the BSON request against the *mongo-service*.
    ///
    /// # Parameters
    /// - `data`: A `Vec<u8>` containing the input BSON to process. This is the BSON
    ///   data serialised to raw bytes.
    ///
    /// # Returns
    /// - `Ok(Vec<u8>)`: A `Vec<u8>` containing the BSON response from the service.
    /// - `Err(_)`: An error variant if the operation fails, with the specific error providing
    ///   details about what went wrong.
    ///
    /// # Errors
    /// This function may return an error under the following conditions:
    /// - If the input data is invalid or corrupted.
    /// - If the processing logic encounters an unexpected failure.
    ///
    /// # Examples
    /// Basic usage:
    /// ```no_run
    /// use bson::doc;
    /// use mongo_service::client::cpp::execute;
    /// let input_data = doc!{}.to_vec().unwrap();
    /// let result = execute(input_data);
    ///
    /// match result {
    ///     Ok(output_data) => println!("Processed output: {:?}", output_data),
    ///     Err(e) => eprintln!("An error occurred: {:?}", e),
    /// }
    /// ```
    ///
    /// # Safety
    /// This function does not perform unsafe operations.
    pub fn execute(data: Vec<u8>) -> Result<Vec<u8>>;
  }
}

impl cpp::Logger
{
  /// Creates a new `Logger` configuration instance with the provided directory and name.
  ///
  /// # Parameters
  /// - `dir`: A string slice representing the directory path where the log files will be stored.
  /// - `name`: A string slice representing the name of the logger or log file.
  ///
  /// # Returns
  /// - Returns a new instance of `Logger`.
  ///
  /// # Defaults
  /// - The `level` is set to `LogLevel::INFO`.
  /// - Logging to the console is disabled (`console: false`).
  ///
  /// # Example
  /// ```
  /// use mongo_service::Logger;
  /// let logger = Logger::new("/var/logs", "application");
  /// ```
  pub fn new(dir: &str, name: &str) -> Self
  {
    cpp::Logger{path: dir.to_string(), name: name.to_string(), level: cpp::LogLevel::INFO, console: false}
  }
}

impl cpp::Pool
{
  /// Creates a new instance of the connection `Pool` configuration with predefined default settings.
  ///
  /// # Default Values
  /// - `initialSize`: 1 - The initial number of connections in the pool.
  /// - `maxPoolSize`: 25 - The maximum number of connections allowed in the pool.
  /// - `maxConnections`: 100 - The maximum total number of database connections that can
  ///   be created, including connections not currently in the pool.
  /// - `maxIdleTimeSeconds`: 300 - The maximum time (in seconds) that a connection can remain idle
  ///   before it is removed from the pool.
  ///
  /// # Returns
  /// A new `Pool` instance with the default configuration.
  ///
  /// # Examples
  /// ```rust
  /// use mongo_service::client::cpp::Pool;
  /// let pool = Pool::new();
  /// ```
  pub fn new() -> Self
  {
    cpp::Pool{initialSize: 1, maxPoolSize: 25, maxConnections: 100, maxIdleTimeSeconds: 300}
  }
}

impl cpp::Configuration
{
  /// Creates a new C++ API `Configuration` instance with the specified application name, host, and port.
  ///
  /// # Arguments
  /// * `app` - A string slice that holds the name of the application.
  /// * `host` - A string slice that represents the host address.
  /// * `port` - An unsigned 16-bit integer representing the port number.
  ///
  /// # Returns
  /// A `Configuration` struct instance initialised with a new connection pool,
  /// and the provided application name, host, and port.
  ///
  /// # Example
  /// ```
  /// use mongo_service::Configuration;
  /// let config = Configuration::new("MyApp", "127.0.0.1", 8080);
  /// ```
  pub fn new(app: &str, host: &str, port: u16) -> Self
  {
    cpp::Configuration{pool: Pool::new(), application: app.to_string(), host: host.to_string(), port}
  }
}
