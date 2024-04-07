/**
 * The notificication system will handle changes
 * in statuses that we subscribe to.
 *
 * For instance, if we would like to issue a
 * notification when a camera is turned off or on,
 * or if the ir sensor has detected a person at the
 * door.
 *
 * We simply subscribe to topics such as:
 * System/ir_sensor/Info
 * Media/video_stream/Info
 *
 * and get the state. if, a running state was issued
 * then we will issue a notification under:
 *
 * System/notification/<time_stamp>
 *
 * {
 *      "event": "ir_sensor",
 *      "ts": <time_stamp>,
 *      "change": {
 *          "state": "running",
 *          "detect": true
 *      }
 * }
 *
 * System/notification/<time_stamp>
 *
 * {
 *      "event": "video_stream",
 *      "ts": <time_stamp>,
 *      "change": {
 *          "state": "running",
 *          "sdp_file": "...",
 *          ...
 *      }
 * }
 *
 * the topics are stored in a map.
 * once a topic is
 *
*/

